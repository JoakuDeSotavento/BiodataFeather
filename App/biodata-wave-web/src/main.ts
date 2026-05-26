import { BleMidiClient } from "./bleMidiClient";
import { parseBleMidiPacket } from "./bleMidiParser";
import { SinePolySynth } from "./instruments/sinePolySynth";
import type { MidiInstrument } from "./instruments/types";
import { PolyPadSynth } from "./polySynth";
import "./style.css";

const app = document.querySelector<HTMLDivElement>("#app");
if (!app) throw new Error("#app");

const hasBle = typeof navigator !== "undefined" && !!navigator.bluetooth;

type InstrumentId = "sine" | "pad";

let instrumentKind: InstrumentId = "sine";
let audioCtx: AudioContext | null = null;
let synth: MidiInstrument | null = null;

function createSynthForContext(ctx: AudioContext): MidiInstrument {
  return instrumentKind === "sine" ? new SinePolySynth(ctx) : new PolyPadSynth(ctx);
}

function applyInstrumentChoice(kind: InstrumentId): void {
  if (kind === instrumentKind && synth) return;
  instrumentKind = kind;
  const wasRunning = synth?.running ?? false;
  const ac = audioCtx;
  const ctxWasRunning = ac?.state === "running";
  synth?.dispose();
  synth = null;
  if (ac) {
    synth = createSynthForContext(ac);
    if (wasRunning && ctxWasRunning) void synth.resume();
  }
  setUi();
}

function ensureSynth(): MidiInstrument {
  if (!audioCtx) {
    audioCtx = new AudioContext();
  }
  if (!synth) {
    synth = createSynthForContext(audioCtx);
  }
  return synth;
}

let connected = false;
let logLines: string[] = [];

function pushLog(line: string): void {
  const ts = new Date().toLocaleTimeString();
  logLines = [`[${ts}] ${line}`, ...logLines].slice(0, 40);
  const el = document.querySelector<HTMLPreElement>("#log");
  if (el) el.textContent = logLines.join("\n");
}

function setUi(): void {
  const st = document.querySelector<HTMLSpanElement>("#conn");
  const dev = document.querySelector<HTMLSpanElement>("#device");
  const btnBle = document.querySelector<HTMLButtonElement>("#btnBle");
  const btnAudio = document.querySelector<HTMLButtonElement>("#btnAudio");
  if (st) st.textContent = connected ? "Conectado" : "Desconectado";
  if (dev) dev.textContent = ble.getDeviceName();
  if (btnBle) {
    btnBle.textContent = connected ? "Desconectar BLE" : "Conectar biodata (BLE MIDI)";
    btnBle.classList.toggle("primary", !connected);
  }
  if (btnAudio) {
    const s = synth;
    btnAudio.textContent =
      s?.running && audioCtx?.state === "running"
        ? "Audio activo"
        : "Activar audio (tocar)";
    btnAudio.disabled = !!(s?.running && audioCtx?.state === "running");
  }
}

const ble = new BleMidiClient({
  onPacket: (data) => {
    const events = parseBleMidiPacket(data);
    if (events.length === 0) return;
    const s = synth;
    if (s?.running) {
      s.handleMidi(events);
    }
    const preview = events
      .map((e) =>
        e.kind === "noteOn"
          ? `On n=${e.note} v=${e.velocity}`
          : e.kind === "noteOff"
            ? `Off n=${e.note}`
            : `CC ${e.controller}=${e.value}`
      )
      .join(" · ");
    pushLog(preview);
  },
  onConnectionChange: (isConnected) => {
    connected = isConnected;
    pushLog(isConnected ? "BLE MIDI listo." : "BLE desconectado.");
    setUi();
  },
  onError: (msg) => {
    pushLog(`Error: ${msg}`);
    setUi();
  },
});

app.innerHTML = `
  <h1>Biodata Wave</h1>
  <p class="sub">Recibe MIDI por Bluetooth Low Energy desde tu Biodata (perfil MIDI BLE) y lo convierte en sonido con instrumentos Web Audio (seno o pad), en la línea de apps tipo plantwave.</p>

  <div class="panel">
    <div class="row">
      <button type="button" class="primary" id="btnAudio">Activar audio (tocar)</button>
      <button type="button" id="btnBle" ${hasBle ? "" : "disabled"}>Conectar biodata (BLE MIDI)</button>
    </div>
    <div class="row instrument-row">
      <label for="instrument">Instrumento</label>
      <select id="instrument" class="touch-control" aria-label="Elegir instrumento de síntesis">
        <option value="sine" selected>Seno (biodata)</option>
        <option value="pad">Pad</option>
      </select>
    </div>
    <p class="status" style="margin-top:0.75rem">Estado BLE: <strong id="conn">Desconectado</strong> · <span id="device">—</span></p>
    ${
      hasBle
        ? ""
        : '<p class="hint">Este navegador no expone Web Bluetooth. En Android usa <strong>Chrome</strong> y abre la app por <strong>HTTPS</strong> o <strong>localhost</strong> al desarrollar.</p>'
    }
    <pre class="log" id="log" aria-live="polite"></pre>
  </div>
  <p class="hint">En el ESP32 activa salida <strong>bleMIDI</strong> y anuncia el servicio MIDI BLE. Activa primero el audio (gesto de usuario); luego conecta el BLE.</p>
`;

document.querySelector("#btnAudio")?.addEventListener("click", async () => {
  const s = ensureSynth();
  await s.resume();
  pushLog("AudioContext en marcha.");
  setUi();
});

document.querySelector("#btnBle")?.addEventListener("click", async () => {
  if (!hasBle) return;
  if (connected) {
    await ble.disconnect();
  } else {
    await ensureSynth().resume();
    await ble.connect();
  }
  setUi();
});

document.querySelector("#instrument")?.addEventListener("change", (ev) => {
  const sel = ev.target as HTMLSelectElement;
  const v = sel.value as InstrumentId;
  if (v !== "sine" && v !== "pad") return;
  applyInstrumentChoice(v);
  pushLog(`Instrumento: ${v === "sine" ? "seno" : "pad"}.`);
});

setUi();
pushLog("Listo. Pulsa «Activar audio» y luego conecta el dispositivo.");
