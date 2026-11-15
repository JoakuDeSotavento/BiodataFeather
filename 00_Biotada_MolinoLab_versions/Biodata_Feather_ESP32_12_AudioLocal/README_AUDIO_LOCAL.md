# Biodata v12 — Audio local

Base: v11 Environmental. Añade un **modo audio local**: el Biodata sintetiza el sonido él mismo y lo saca por el **minijack MIDI** (GPIO 39), sin ordenador ni sintetizador externo.

## Conmutar MIDI / audio

- **Mantener el botón 3 s** (en modo normal): los 5 LEDs parpadean 3 veces, suelta el botón y el Biodata se reinicia en el otro modo. El modo queda guardado.
- En modo audio, al arrancar suena un acorde corto (La-Mi-La) para confirmar que la salida funciona. Si el LED rojo se queda fijo, la salida de audio no pudo iniciarse.
- El modo audio es **exclusivo**: se apagan WiFi, BLE MIDI, USB MIDI y MIDI por cable. El USB sigue sirviendo para el monitor serie y para cargar firmware.
- Reset de fábrica (botón pulsado al encender) vuelve a modo MIDI.

## Menú en modo audio

| LED | Modo MIDI | Modo audio |
|---|---|---|
| Rojo | Escala | Escala (igual) |
| Amarillo | Canal MIDI | **Sonido**: Pad, Pluck, Bell, Drone (1 a 4 en binario) |
| Verde | WiFi off / config / on | Solo off/on se guarda; config no abre portal en audio |
| Azul | BLE | Solo se guarda; se aplica al volver a MIDI |
| Blanco | Nota raíz | Nota raíz (igual) |

El sonido cambia en vivo al girar el potenciómetro, igual que la escala.

## Elegir la salida (`AUDIO_OUTPUT` en `AudioLocal.cpp`)

| Valor | Salida | Hardware |
|---|---|---|
| `AUDIO_OUT_PDM_JACK` (por defecto) | PDM 1 bit por DMA en GPIO 39 | Ninguno: cable de audio en el minijack MIDI |
| `AUDIO_OUT_SDM_JACK` | Sigma-delta 8 bits por timer en GPIO 39 | Ninguno; reserva si el PDM da problemas |
| `AUDIO_OUT_PDM_PINS` | PDM: CLK = D9, DATA = D11 | Módulo/amp PDM externo |
| `AUDIO_OUT_I2S_DAC` | I2S: BCK = D9, LRCK/WSEL = D10, DIN = D11 | UDA1334A |

Placa recomendada: **Adafruit Feather ESP32-S3 2MB PSRAM** (el delay usa 1 s de buffer en PSRAM). Sin PSRAM funciona igual con un delay más corto. Opciones de compilación iguales que v11 (USB-OTG TinyUSB, USB CDC On Boot Enabled).

## Fase A — prueba sin hardware (minijack MIDI)

1. Compilar con `AUDIO_OUT_PDM_JACK`, pasar a modo audio.
2. Cable jack 3,5 mm estéreo normal desde el minijack MIDI a la **entrada de línea** de unos altavoces activos o una mesa. Empezar con volumen bajo.
3. El audio sale **en un solo canal** (el contacto que lleva el GPIO 39); el otro puede llevar la alimentación del MIDI TRS y no suena. Si no se oye nada, probar el otro canal o un adaptador mono.
4. La señal lleva un nivel de continua (~1,65 V). Casi todas las entradas de línea la bloquean; si alguna no, poner un condensador de 1–10 µF en serie.

Qué esperar: suena, pero con algo de soplido y menos graves que un DAC real. Si el resultado es aceptable para el uso previsto, no hace falta añadir nada.

## Fase B — PDM a pines libres

Compilar con `AUDIO_OUT_PDM_PINS` y conectar un módulo PDM a D9 (CLK) y D11 (DATA). Sirve para descartar que el ruido venga del cableado del jack y para medir la carga de CPU.

Con el monitor serie abierto, cada 10 s se imprime una línea con este formato (valores de ejemplo):

```
Audio CPU: media 410 us (14%), pico 520 us (18%) por bloque de 2902 us
```

Por encima de ~70 % en el pico hay riesgo de cortes; bajar voces o simplificar el sonido.

## Fase C — UDA1334A (sin PCB nueva)

Compilar con `AUDIO_OUT_I2S_DAC` (en `AudioLocal.cpp`: `#define AUDIO_OUTPUT AUDIO_OUT_I2S_DAC`) y cablear con jumpers:

| UDA1334A (Adafruit u compatible) | Feather ESP32-S3 |
|---|---|
| VIN | 3V (o USB; el módulo suele aceptar 3,3–5 V) |
| GND | GND |
| BCLK | **D9** (GPIO 9) |
| WSEL / LRCLK / WS | **D10** (GPIO 10) |
| DIN / DATA | **D11** (GPIO 11) |
| LOUT / ROUT (+ AGND si existe) | minijack estéreo nuevo: tip = L, ring = R, sleeve = GND |

Notas del UDA1334A:
- No hace falta pin SCK/MCLK: el módulo usa el PLL a partir de BCLK.
- Formato I2S (no left-justified): en placas Adafruit, SF0/SF1 suelen ir por defecto en I2S.
- Salida **estéreo** a nivel de línea (mismo audio en L y R; no es panorama entre voces). Va directa a altavoces activos o mesa. Para **auriculares** hace falta un amp de auriculares.

El protocolo I2S del firmware es el mismo que se había previsto para el PCM5102: **no hay que cambiar pines ni lógica**, solo el DAC físico.

## Archivos que cambian respecto a v11

- `AudioLocal.cpp` — motor (5 voces, 2 osciladores, filtro 12 dB, ADSR, delay) y salidas.
- `Biodata_Feather_ESP32_12_AudioLocal.ino` — modo audio, EEPROM 6 (modo) y 7 (sonido), menú y pulsación larga.
- `Main.ino` — arranque en modo audio, estadísticas de CPU.
- `MIDI.ino` — las notas, sus finales y el CC también van al sintetizador.
