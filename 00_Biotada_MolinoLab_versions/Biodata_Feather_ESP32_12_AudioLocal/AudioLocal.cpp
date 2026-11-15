// ============================================================================
// Audio local - motor de síntesis polifónico + salidas de audio (ESP32-S3)
// ============================================================================
// El loop() de Arduino (core 1) solo encola eventos de nota/CC. Una tarea de
// audio en el core 0 sintetiza bloques y los empuja a la salida elegida:
//
//   AUDIO_OUT_PDM_JACK : I2S PDM TX con el dato en GPIO 39 (tip del minijack
//                        MIDI). Sigma-delta 1 bit a ~3 MHz por DMA; el cable y
//                        la entrada de línea hacen de filtro paso-bajo.
//   AUDIO_OUT_SDM_JACK : sigma-delta 8 bits en GPIO 39 alimentado por timer
//                        (sin DMA). Reserva por si el PDM da problemas.
//   AUDIO_OUT_PDM_PINS : PDM TX en pines libres (CLK D9 / DATA D11) para un
//                        módulo/amp PDM externo.
//   AUDIO_OUT_I2S_DAC  : I2S estándar a un DAC UDA1334A:
//                        BCK D9, LRCK/WSEL D10, DIN D11.
// ============================================================================

#include <Arduino.h>
#include <math.h>
#include <ESP_I2S.h>
#include "driver/sdm.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define AUDIO_OUT_PDM_JACK 0
#define AUDIO_OUT_SDM_JACK 1
#define AUDIO_OUT_PDM_PINS 2
#define AUDIO_OUT_I2S_DAC 3

#ifndef AUDIO_OUTPUT
#define AUDIO_OUTPUT AUDIO_OUT_PDM_JACK
#endif

// Pines (Feather ESP32-S3). 39 = minijack MIDI; 9/10/11 = D9/D10/D11 libres.
#define AUDIO_JACK_PIN 39
#define AUDIO_PDM_CLK_PIN 9  // en PDM_JACK se genera pero no se conecta a nada
#define AUDIO_PDM_DATA_PIN 11
#define AUDIO_I2S_BCK_PIN 9
#define AUDIO_I2S_LRCK_PIN 10
#define AUDIO_I2S_DIN_PIN 11

#define AUDIO_BLOCK 128
#define AUDIO_VOICES 5
#define AUDIO_MASTER_GAIN 0.6f

// ---------------------------------------------------------------------------
// Patches
// ---------------------------------------------------------------------------
struct SynthPatch {
  const char *name;
  float attack, decay, sustain, release;  // segundos / nivel
  float sineMix;                          // 0 = sierra, 1 = seno
  float osc2Ratio, osc2Level;             // segundo oscilador
  float cutoff, cutoffEnv, cutoffCC;      // Hz
  float delayTime, delayFeedback, delayMix;
};

static const SynthPatch patches[] = {
  { "Pad", 0.9f, 1.5f, 0.75f, 2.8f, 0.35f, 1.006f, 0.8f, 500.f, 1400.f, 1800.f, 0.42f, 0.45f, 0.30f },
  { "Pluck", 0.003f, 0.6f, 0.0f, 0.5f, 0.0f, 2.0f, 0.25f, 300.f, 4200.f, 2000.f, 0.33f, 0.35f, 0.25f },
  { "Bell", 0.002f, 1.8f, 0.0f, 2.2f, 1.0f, 2.76f, 0.45f, 6000.f, 0.f, 0.f, 0.50f, 0.40f, 0.30f },
  { "Drone", 2.5f, 2.0f, 0.9f, 4.5f, 0.6f, 0.501f, 0.7f, 350.f, 600.f, 1200.f, 0.70f, 0.55f, 0.35f },
};
static const uint8_t patchCount = sizeof(patches) / sizeof(patches[0]);

// ---------------------------------------------------------------------------
// Voces
// ---------------------------------------------------------------------------
enum EnvStage : uint8_t { ENV_IDLE, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

struct Voice {
  EnvStage stage = ENV_IDLE;
  float env = 0.f;
  float phase1 = 0.f, phase2 = 0.f;
  float inc1 = 0.f, inc2 = 0.f;
  float amp = 0.f;
  float velNorm = 0.f;
  float lp1 = 0.f, lp2 = 0.f;
};

enum EventType : uint8_t { EV_NOTE_ON, EV_NOTE_OFF, EV_CONTROL, EV_PATCH };
struct SynthEvent {
  EventType type;
  uint8_t voice, note, value;
};

#define SINE_SIZE 1024
static float sineTable[SINE_SIZE];

static Voice voices[AUDIO_VOICES];
static const SynthPatch *patch = &patches[0];
static uint32_t sampleRate = 44100;
static float ccNorm = 0.f;
static float attackInc, decayCoef, releaseCoef;

static float *delayBuf = nullptr;
static uint32_t delayLen = 0, delayPos = 0;
static float delayLp = 0.f;

static QueueHandle_t eventQueue = nullptr;
static volatile bool audioRunning = false;
static volatile uint32_t statBlocks = 0, statRenderUs = 0, statMaxRenderUs = 0;

static inline float sineLookup(float phase) {
  return sineTable[(uint32_t)(phase * SINE_SIZE) & (SINE_SIZE - 1)];
}

// PolyBLEP: suaviza el salto de la sierra para reducir aliasing
static inline float polyBlep(float t, float dt) {
  if (t < dt) {
    t /= dt;
    return t + t - t * t - 1.f;
  }
  if (t > 1.f - dt) {
    t = (t - 1.f) / dt;
    return t * t + t + t + 1.f;
  }
  return 0.f;
}

static inline float oscSample(float phase, float inc) {
  float saw = 2.f * phase - 1.f - polyBlep(phase, inc);
  return saw + (sineLookup(phase) - saw) * patch->sineMix;
}

static void computeEnvelopeCoefs() {
  const float sr = (float)sampleRate;
  attackInc = 1.f / fmaxf(patch->attack * sr, 1.f);
  // -60 dB en el tiempo indicado
  decayCoef = expf(logf(0.001f) / fmaxf(patch->decay * sr, 1.f));
  releaseCoef = expf(logf(0.001f) / fmaxf(patch->release * sr, 1.f));
}

static void applyPatch(uint8_t index) {
  if (index >= patchCount) index = 0;
  patch = &patches[index];
  computeEnvelopeCoefs();
}

static void handleEvent(const SynthEvent &ev) {
  if (ev.type == EV_PATCH) {
    applyPatch(ev.value);
    return;
  }
  if (ev.type == EV_CONTROL) {
    ccNorm = ev.value / 127.f;
    return;
  }
  if (ev.voice >= AUDIO_VOICES) return;
  Voice &v = voices[ev.voice];
  if (ev.type == EV_NOTE_ON) {
    const float freq = 440.f * powf(2.f, (ev.note - 69) / 12.f);
    v.inc1 = freq / sampleRate;
    v.inc2 = fminf(freq * patch->osc2Ratio / sampleRate, 0.45f);
    v.velNorm = ev.value / 127.f;
    v.amp = 0.25f + 0.75f * v.velNorm;
    v.stage = ENV_ATTACK;  // retrigger desde el nivel actual: sin clicks
  } else if (v.stage != ENV_IDLE) {
    v.stage = ENV_RELEASE;
  }
}

static void renderBlock(float *out, int n) {
  SynthEvent ev;
  while (xQueueReceive(eventQueue, &ev, 0) == pdTRUE) handleEvent(ev);

  memset(out, 0, sizeof(float) * n);
  const float twoPiOverSr = 6.2831853f / sampleRate;

  for (int vi = 0; vi < AUDIO_VOICES; vi++) {
    Voice &v = voices[vi];
    if (v.stage == ENV_IDLE) continue;

    // Filtro paso-bajo 12 dB (dos polos simples), cutoff actualizado por bloque
    float cutoff = patch->cutoff + patch->cutoffEnv * v.env * v.velNorm + patch->cutoffCC * ccNorm;
    cutoff = fminf(cutoff, sampleRate * 0.45f);
    const float g = 1.f - expf(-cutoff * twoPiOverSr);

    for (int i = 0; i < n; i++) {
      switch (v.stage) {
        case ENV_ATTACK:
          v.env += attackInc;
          if (v.env >= 1.f) {
            v.env = 1.f;
            v.stage = ENV_DECAY;
          }
          break;
        case ENV_DECAY:
          v.env = patch->sustain + (v.env - patch->sustain) * decayCoef;
          if (v.env - patch->sustain < 0.001f) {
            v.env = patch->sustain;
            v.stage = patch->sustain > 0.f ? ENV_SUSTAIN : ENV_IDLE;
          }
          break;
        case ENV_RELEASE:
          v.env *= releaseCoef;
          if (v.env < 0.0005f) {
            v.env = 0.f;
            v.stage = ENV_IDLE;
          }
          break;
        default: break;
      }

      float s = oscSample(v.phase1, v.inc1) + patch->osc2Level * oscSample(v.phase2, v.inc2);
      v.phase1 += v.inc1;
      if (v.phase1 >= 1.f) v.phase1 -= 1.f;
      v.phase2 += v.inc2;
      if (v.phase2 >= 1.f) v.phase2 -= 1.f;

      v.lp1 += g * (s - v.lp1);
      v.lp2 += g * (v.lp1 - v.lp2);
      out[i] += v.lp2 * v.env * v.amp;
    }
  }

  // Delay con realimentación filtrada (buffer en PSRAM si existe)
  const uint32_t dTime = delayLen ? (uint32_t)fminf(patch->delayTime * sampleRate, delayLen - 1) : 0;
  const float mix = patch->delayMix + 0.15f * ccNorm;
  for (int i = 0; i < n; i++) {
    float dry = out[i] * 0.22f;
    float wet = 0.f;
    if (delayLen) {
      uint32_t readPos = (delayPos + delayLen - dTime) % delayLen;
      wet = delayBuf[readPos];
      delayLp += 0.35f * (wet - delayLp);
      delayBuf[delayPos] = dry + delayLp * patch->delayFeedback;
      delayPos = (delayPos + 1) % delayLen;
    }
    float x = (dry + wet * mix) * AUDIO_MASTER_GAIN * 2.f;
    // Saturación suave (aprox. tanh) para no recortar en seco
    x = x * (27.f + x * x) / (27.f + 9.f * x * x);
    out[i] = fmaxf(-1.f, fminf(1.f, x));
  }
}

// ---------------------------------------------------------------------------
// Salidas
// ---------------------------------------------------------------------------
#if AUDIO_OUTPUT == AUDIO_OUT_SDM_JACK

#define SDM_SAMPLE_RATE 22222  // 1 MHz / 45
#define SDM_RING 1024
static sdm_channel_handle_t sdmChan = nullptr;
static hw_timer_t *sdmTimer = nullptr;
static volatile int8_t sdmRing[SDM_RING];
static volatile uint32_t sdmHead = 0, sdmTail = 0;

static void IRAM_ATTR sdmIsr() {
  if (sdmTail != sdmHead) {
    sdm_channel_set_pulse_density(sdmChan, sdmRing[sdmTail]);
    sdmTail = (sdmTail + 1) & (SDM_RING - 1);
  }
}

static bool sinkBegin() {
  sampleRate = SDM_SAMPLE_RATE;
  sdm_config_t cfg = {};
  cfg.gpio_num = AUDIO_JACK_PIN;
  cfg.clk_src = SDM_CLK_SRC_DEFAULT;
  cfg.sample_rate_hz = 10 * 1000 * 1000;
  if (sdm_new_channel(&cfg, &sdmChan) != ESP_OK) return false;
  if (sdm_channel_enable(sdmChan) != ESP_OK) return false;
  sdmTimer = timerBegin(1000000);
  if (!sdmTimer) return false;
  timerAttachInterrupt(sdmTimer, &sdmIsr);
  timerAlarm(sdmTimer, 45, true, 0);
  return true;
}

static void sinkWrite(const float *buf, int n) {
  for (int i = 0; i < n; i++) {
    uint32_t next = (sdmHead + 1) & (SDM_RING - 1);
    while (next == sdmTail) vTaskDelay(1);
    sdmRing[sdmHead] = (int8_t)(buf[i] * 127.f);
    sdmHead = next;
  }
}

static const char *sinkName() {
  return "SDM GPIO39 (minijack)";
}

#else  // salidas I2S (PDM o estándar), todas por DMA

static I2SClass i2s;
#if AUDIO_OUTPUT == AUDIO_OUT_I2S_DAC
static int16_t i2sBuf[AUDIO_BLOCK * 2];  // UDA1334A espera tramas estéreo
#else
static int16_t i2sBuf[AUDIO_BLOCK];
#endif

static bool sinkBegin() {
  sampleRate = 44100;
#if AUDIO_OUTPUT == AUDIO_OUT_I2S_DAC
  i2s.setPins(AUDIO_I2S_BCK_PIN, AUDIO_I2S_LRCK_PIN, AUDIO_I2S_DIN_PIN);
  return i2s.begin(I2S_MODE_STD, sampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
#elif AUDIO_OUTPUT == AUDIO_OUT_PDM_PINS
  i2s.setPinsPdmTx(AUDIO_PDM_CLK_PIN, AUDIO_PDM_DATA_PIN);
  return i2s.begin(I2S_MODE_PDM_TX, sampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
#else
  i2s.setPinsPdmTx(AUDIO_PDM_CLK_PIN, AUDIO_JACK_PIN);
  return i2s.begin(I2S_MODE_PDM_TX, sampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
#endif
}

static void sinkWrite(const float *buf, int n) {
#if AUDIO_OUTPUT == AUDIO_OUT_I2S_DAC
  for (int i = 0; i < n; i++) {
    int16_t s = (int16_t)(buf[i] * 32767.f);
    i2sBuf[2 * i] = s;
    i2sBuf[2 * i + 1] = s;
  }
  i2s.write((const uint8_t *)i2sBuf, sizeof(int16_t) * 2 * n);
#else
  for (int i = 0; i < n; i++) i2sBuf[i] = (int16_t)(buf[i] * 32767.f);
  i2s.write((const uint8_t *)i2sBuf, sizeof(int16_t) * n);
#endif
}

static const char *sinkName() {
#if AUDIO_OUTPUT == AUDIO_OUT_I2S_DAC
  return "I2S DAC UDA1334A (D9/D10/D11)";
#elif AUDIO_OUTPUT == AUDIO_OUT_PDM_PINS
  return "PDM pines (CLK D9 / DATA D11)";
#else
  return "PDM GPIO39 (minijack)";
#endif
}

#endif

// ---------------------------------------------------------------------------
// Tarea de audio y API pública
// ---------------------------------------------------------------------------
static void audioTask(void *) {
  static float block[AUDIO_BLOCK];
  for (;;) {
    uint32_t t0 = micros();
    renderBlock(block, AUDIO_BLOCK);
    uint32_t dt = micros() - t0;
    statRenderUs += dt;
    if (dt > statMaxRenderUs) statMaxRenderUs = dt;
    statBlocks++;
    sinkWrite(block, AUDIO_BLOCK);
  }
}

bool audioBegin(uint8_t patchIndex) {
  if (audioRunning) return true;
  for (int i = 0; i < SINE_SIZE; i++) sineTable[i] = sinf(6.2831853f * i / SINE_SIZE);

  eventQueue = xQueueCreate(32, sizeof(SynthEvent));
  if (!eventQueue) return false;

  if (!sinkBegin()) {
    Serial.println(F("Audio: no se pudo iniciar la salida"));
    return false;
  }
  applyPatch(patchIndex);

  // 1 s de delay en PSRAM; sin PSRAM, 0.35 s en RAM interna
  uint32_t wanted = sampleRate;
  if (psramFound()) delayBuf = (float *)heap_caps_calloc(wanted, sizeof(float), MALLOC_CAP_SPIRAM);
  if (!delayBuf) {
    wanted = (uint32_t)(sampleRate * 0.35f);
    delayBuf = (float *)heap_caps_calloc(wanted, sizeof(float), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  delayLen = delayBuf ? wanted : 0;

  Serial.printf("Audio local: %s, %lu Hz, patch %s, delay %lu muestras (%s)\n", sinkName(), (unsigned long)sampleRate,
                patch->name, (unsigned long)delayLen, psramFound() ? "PSRAM" : "RAM interna");

  audioRunning = xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 5, nullptr, 0) == pdPASS;
  return audioRunning;
}

static void pushEvent(EventType type, uint8_t voice, uint8_t note, uint8_t value) {
  if (!audioRunning) return;
  SynthEvent ev = { type, voice, note, value };
  xQueueSend(eventQueue, &ev, 0);
}

void audioNoteOn(uint8_t voice, uint8_t note, uint8_t velocity) {
  pushEvent(EV_NOTE_ON, voice, note, velocity);
}

void audioNoteOff(uint8_t voice) {
  pushEvent(EV_NOTE_OFF, voice, 0, 0);
}

void audioControl(uint8_t value) {
  pushEvent(EV_CONTROL, 0, 0, value & 0x7F);
}

void audioSetPatch(uint8_t index) {
  pushEvent(EV_PATCH, 0, 0, index);
}

uint8_t audioPatchCount() {
  return patchCount;
}

const char *audioPatchName(uint8_t index) {
  return patches[index < patchCount ? index : 0].name;
}

// Carga de CPU de la síntesis: tiempo medio y máximo de render frente al
// tiempo real de un bloque. Por encima de ~70 % hay riesgo de cortes.
void audioPrintStats() {
  if (!audioRunning || statBlocks == 0) return;
  const float blockUs = 1e6f * AUDIO_BLOCK / sampleRate;
  const float avg = (float)statRenderUs / statBlocks;
  Serial.printf("Audio CPU: media %.0f us (%.0f%%), pico %lu us (%.0f%%) por bloque de %.0f us\n", avg, 100.f * avg / blockUs,
                (unsigned long)statMaxRenderUs, 100.f * statMaxRenderUs / blockUs, blockUs);
  statRenderUs = 0;
  statBlocks = 0;
  statMaxRenderUs = 0;
}
