# 📡 MQTT Buffer para Notas MIDI → InfluxDB + Sensores Ambientales

## 🎯 ¿Qué hace esto?

Esta implementación añade:
1. **Sistema de buffering** que captura todas las notas MIDI generadas por el análisis de biodatos y las envía en grupos vía MQTT a InfluxDB.
2. **Sensores ambientales** (BME688 + LTR329) que leen temperatura, humedad, presión, gas, altitud y luz, enviando datos cada 5 minutos vía MQTT.

**✅ NO modifica ninguna funcionalidad MIDI existente** (USB, BLE, WiFi/RTP, Serial).

---

## ⚙️ Configuración

### 1️⃣ Librerías Requeridas

**⚠️ IMPORTANTE**: Instala estas librerías ANTES de compilar:

En el Arduino IDE → Herramientas → Administrar Bibliotecas:

- **PubSubClient** (v2.8+) - Cliente MQTT por Nick O'Leary
- **ArduinoJson** (v6.x) - Por Benoit Blanchon
- **Adafruit LTR329 LTR303 Library** - Para sensor de luz LTR329
- **DFRobot_BME68x** - Para sensor ambiental BME688

Si no las instalas, obtendrás errores de compilación.

### 2️⃣ Credenciales - Archivo `secrets.h`

Crea el archivo `secrets.h` basándote en `secrets_sample.h.txt` y edita con tus credenciales:

```cpp
// WiFi - Soporte para hasta 3 redes (se conecta a la primera disponible)
#define WIFI_SSID "Wifi1"
#define WIFI_PASSWORD "password"

#define WIFI_SSID_2 "Wifi2"        // Opcional: Segunda red WiFi
#define WIFI_PASSWORD_2 "password"

#define WIFI_SSID_3 "Wifi3"        // Opcional: Tercera red WiFi
#define WIFI_PASSWORD_3 "password"

// MQTT
#define MQTT_BROKER "mqtt.sinfoniabiotica.xyz"
#define MQTT_PORT 1883
#define MQTT_USER "user"
#define MQTT_PASSWORD "password"

// Sensor ID (opcional - se genera automáticamente desde MAC si no se define)
//#define SENSOR_ID_MANUAL "mnyumbuni_00"

// Topics MQTT
#define MQTT_BASE_TOPIC "biodata_raw"
#define MQTT_ENV_TOPIC "environmental"
```

**Notas importantes:**
- Si no defines `WIFI_SSID_2` o `WIFI_PASSWORD_2`, se ignorarán (puedes dejar vacíos `""`)
- Si no defines `SENSOR_ID_MANUAL`, el ID se generará automáticamente como `biodata_XXXXXX` desde la dirección MAC
- El sistema intentará conectarse a las redes WiFi en orden (1, 2, 3) hasta encontrar una disponible

### 3️⃣ Sensor ID

Por defecto, el ID se genera automáticamente desde la dirección MAC:
- Formato: `biodata_XXXXXX` (donde XXXXXX es un identificador único basado en la MAC)

Si prefieres un ID fijo, descomenta en `secrets.h`:
```cpp
#define SENSOR_ID_MANUAL "mnyumbuni_00"
```

Este ID se usa tanto para:
- **Datos MIDI**: `biodata_raw/{SENSOR_ID}/midi`
- **Datos ambientales**: `environmental/{SENSOR_ID}`

---

## 🚀 Uso

### Activación Automática

El buffer MQTT y los sensores ambientales se activan automáticamente cuando **WiFi está ON** en el menú.

**Orden de inicialización (crítico):**
1. Sensores ambientales (BME688 + LTR329)
2. WiFi y conexión MQTT
3. Buffer MIDI

Este orden es importante para evitar problemas de inicialización I2C.

### Menú LED (mismo que antes):

1. **LED Rojo** - MIDI Scale
2. **LED Amarillo** - MIDI Channel  
3. **LED Verde** - WiFi ON/OFF ← Activa/desactiva buffer MQTT y sensores ambientales
4. **LED Azul** - Bluetooth ON/OFF

### Sensores Ambientales

Los sensores se inicializan automáticamente cuando WiFi está activo:
- **BME688**: Temperatura, humedad, presión, gas, altitud
- **LTR329**: Luz visible + infrarroja, solo infrarroja

**Intervalo de lectura**: Cada 5 minutos (300 segundos)

Si un sensor no está disponible, el sistema continúa funcionando con los sensores disponibles. Si ningún sensor está disponible, el módulo ambiental se deshabilita pero el sistema MIDI sigue funcionando normalmente.

### Configuración del Buffer

En `MQTTInflux.ino`:

```cpp
#define BUFFER_SEND_INTERVAL 10000  // 10 segundos (ajustable)
#define MIDI_BUFFER_SIZE 100        // Máx. 100 notas en buffer
#define ENABLE_RAW_LOGGING 1        // 0 para desactivar el registro crudo
#define RAW_BLOCK_QUEUE_SIZE 6      // Máx. 6 bloques crudos pendientes
#define MQTT_SEND_MAX_RETRIES 1     // Reintentos antes de descartar buffer
```

### Configuración de Sensores Ambientales

En `Environmental.ino`:

```cpp
const unsigned long ENVIRONMENTAL_READ_INTERVAL = 300000;  // 5 minutos (300000 ms)
```

**Nota**: Los sensores se configuran automáticamente con valores óptimos:
- **LTR329**: Gain 2, Integration Time 100ms, Measurement Rate 200ms
- **BME688**: Gas heater a 360°C durante 100ms

---

## 📊 Formato de Datos MQTT

### Topics MQTT

1. **Datos MIDI**: `biodata_raw/{SENSOR_ID}/midi`
2. **Datos ambientales**: `environmental/{SENSOR_ID}`

### Payload JSON - Datos MIDI

```json
{
  "sid": "biodata_a1b2c3",
  "ts": 1234567890,
  "c": 3,
  "rc": 1,
  "notes": [
    {"t": 1001, "n": 60, "v": 90, "d": 500},
    {"t": 1150, "n": 62, "v": 85, "d": 450},
    {"t": 1300, "n": 64, "v": 95, "d": 600}
  ],
  "raw_blocks": [
    {
      "t": 1234500,
      "max": 914,
      "min": 842,
      "avg": 873,
      "std": 23.4,
      "delta": 72,
      "threshold": 1.92
    }
  ]
}
```

**Campos de notas (abreviados para reducir tamaño):**
- `t` — timestamp relativo (`millis()`) cuando se capturó la nota
- `n` — nota MIDI (0-127)
- `v` — velocity (0-127)
- `d` — duración (ms)

**Campos de bloques crudos (`raw_blocks`):**
- `t` — timestamp relativo (`millis()`) cuando se finalizó el bloque
- `max`, `min`, `avg`, `std`, `delta` — métricas básicas del bloque crudo
- `threshold` — valor de `threshold` vigente al calcular el bloque

**Campos de metadata (abreviados):**
- `sid` — sensor_id
- `ts` — timestamp
- `c` — count (número de notas)
- `rc` — raw_count (número de bloques crudos)

### Payload JSON - Datos Ambientales

```json
{
  "temperatura": 23.45,
  "presion": 101325.0,
  "humedad": 65.2,
  "gas": 125000.0,
  "altitud": 525.0,
  "visible_ir": 1234,
  "infrarrojo": 567
}
```

**Campos ambientales:**
- `temperatura` — Temperatura en °C (BME688)
- `presion` — Presión atmosférica en Pa (BME688)
- `humedad` — Humedad relativa en % (BME688)
- `gas` — Resistencia del sensor de gas en Ω (BME688)
- `altitud` — Altitud aproximada en metros (BME688)
- `visible_ir` — Luz visible + infrarroja (LTR329)
- `infrarrojo` — Solo luz infrarroja (LTR329)

**Nota**: Los datos ambientales se envían cada 5 minutos automáticamente cuando WiFi está activo.

---

## 🔍 Monitoreo Serial

Con `debugSerial = 1`, verás mensajes como:

```
=== Inicializando Sensores Ambientales ===
✓ LTR329 OK
✓ BME68x OK
=== Sensores Ambientales Listos ===
LTR329: ✓ Disponible
BME688: ✓ Disponible
Intervalo de lectura: 300 segundos

=== MQTT Buffer Setup ===
Sensor ID: biodata_a1b2c3
MQTT Broker: mqtt.sinfoniabiotica.xyz
Buffer interval: 10 seconds

Conectando MQTT... intento 1
✓ MQTT conectado

--- Lectura Ambiental ---
Temp: 23.45 °C
Pres: 101325 Pa
Hum: 65.20 %
Gas: 125000 Ω
Alt: 525.00 m
Visible+IR: 1234
Infrared: 567
✓ Datos ambientales enviados a: environmental/biodata_a1b2c3

✓ MQTT: Enviadas 12 notas (458 bytes)
```

---

## 🛠️ Solución de Problemas

### El buffer no envía nada

✅ **Normal**: Si no se generan notas **ni** bloques crudos, no se publica nada (comportamiento esperado)

### MQTT no conecta

1. Verifica credenciales en `secrets.h`
2. Confirma que el broker es accesible
3. Revisa el Serial Monitor para errores
4. Verifica que WiFi esté conectado (el LED verde debe estar activo)

### Buffer se llena muy rápido

- Aumenta `BUFFER_SEND_INTERVAL` (ej. 5000 = 5 segundos)
- O aumenta `MIDI_BUFFER_SIZE` (ej. 150)

### Sensores ambientales no funcionan

1. **Verifica conexión I2C**: Los sensores deben estar conectados al bus I2C del ESP32
2. **Verifica alimentación**: Asegúrate de que los sensores tengan alimentación adecuada
3. **Revisa Serial Monitor**: Busca mensajes como "✗ LTR329 no encontrado" o "✗ BME68x no encontrado"
4. **Orden de inicialización**: Los sensores se inicializan ANTES de WiFi (esto es crítico)
5. **Delay después de Serial.begin()**: Debe haber un `delay(1000)` después de `Serial.begin()` para estabilizar I2C

### WiFi no se conecta a ninguna red

1. Verifica que al menos `WIFI_SSID` y `WIFI_PASSWORD` estén correctos en `secrets.h`
2. El sistema intentará conectarse a las redes en orden (1, 2, 3)
3. Si ninguna red está disponible, WiFi se desactivará después de 15 segundos
4. Puedes dejar `WIFI_SSID_2` y `WIFI_SSID_3` vacíos si solo usas una red

### Datos ambientales no se envían

1. Verifica que los sensores estén inicializados correctamente (mensaje "✓ Sensores Ambientales Listos")
2. Verifica que MQTT esté conectado
3. Los datos se envían cada 5 minutos automáticamente
4. Revisa el Serial Monitor para ver si hay errores de envío

---

## 📝 Archivos del Proyecto

### ✨ Nuevos:
- `secrets.h` - Credenciales centralizadas (WiFi, MQTT, Topics)
- `secrets_sample.h.txt` - Plantilla de ejemplo para `secrets.h`
- `MQTTInflux.ino` - Toda la lógica de buffer/MQTT para datos MIDI
- `Environmental.ino` - Lógica de sensores ambientales (BME688 + LTR329)

### 📌 Archivos principales:
- `Biodata_Feather_ESP32_11_Environmental.ino` - Archivo principal con includes y configuración
- `Main.ino` - Setup y loop principal
- `MIDI.ino` - Lógica MIDI
- `SampleAnalysis.ino` - Análisis de muestras de biodatos
- `Scale.ino` - Escalas musicales
- `ERRORES_Y_SOLUCIONES.md` - Documentación de problemas y soluciones

---

## 🔐 Seguridad

⚠️ **IMPORTANTE**: No subas `secrets.h` a repositorios públicos.

Agrega a `.gitignore`:
```
secrets.h
```

---

## 🎵 Flujo de Datos

### Datos MIDI (Biodatos)
```
Planta → Galvanómetro → ESP32 → Análisis → Nota MIDI
                                              ↓
                                    ┌─────────┴──────────┐
                                    ↓                    ↓
                         [USB/BLE/WiFi-RTP/Serial]   [Buffer MQTT]
                         (tiempo real, sin cambios)  (cada 10s agrupado)
                                                          ↓
                                                     MQTT Broker
                                                          ↓
                                                      InfluxDB
```

### Datos Ambientales
```
BME688 + LTR329 → ESP32 → Lectura cada 5 min → MQTT
                                              ↓
                                    environmental/{SENSOR_ID}
                                              ↓
                                         MQTT Broker
                                              ↓
                                          InfluxDB
```

---

## 📖 Más Información

- Plan técnico completo: `00_Biotada_MolinoLab_versions/coms/2_PLAN.md`
- Configuración completa: `secrets.h` (usar `secrets_sample.h.txt` como plantilla)
- Lógica del buffer MIDI: `MQTTInflux.ino`
- Lógica de sensores ambientales: `Environmental.ino`
- Solución de problemas: `ERRORES_Y_SOLUCIONES.md`

---

## 🔐 Seguridad y Configuración

### Crear `secrets.h`

1. Copia `secrets_sample.h.txt` a `secrets.h`
2. Edita `secrets.h` con tus credenciales reales
3. **NUNCA** subas `secrets.h` a repositorios públicos
4. Agrega `secrets.h` a `.gitignore`

### Estructura de `secrets.h`

```cpp
// WiFi (hasta 3 redes)
#define WIFI_SSID "red_principal"
#define WIFI_PASSWORD "password_principal"
#define WIFI_SSID_2 "red_secundaria"      // Opcional
#define WIFI_PASSWORD_2 "password_secundaria"
#define WIFI_SSID_3 "red_terciaria"        // Opcional
#define WIFI_PASSWORD_3 "password_terciaria"

// MQTT
#define MQTT_BROKER "mqtt.sinfoniabiotica.xyz"
#define MQTT_PORT 1883
#define MQTT_USER "usuario_mqtt"
#define MQTT_PASSWORD "password_mqtt"

// Sensor ID (opcional - auto-generado desde MAC si no se define)
//#define SENSOR_ID_MANUAL "mi_sensor_01"

// Topics MQTT
#define MQTT_BASE_TOPIC "biodata_raw"
#define MQTT_ENV_TOPIC "environmental"
```

---

**Versión**: 2.0  
**Fecha**: Diciembre 2024  
**Buffer Interval**: 10 segundos  
**Buffer Size**: 100 notas máx.  
**Intervalo Ambiental**: 5 minutos (300 segundos)  
**Sensores**: BME688 (temp, humedad, presión, gas, altitud) + LTR329 (luz visible/IR)

