# 📡 MQTT Buffer para Notas MIDI → InfluxDB

## 🎯 ¿Qué hace esto?

Esta implementación añade un **sistema de buffering** que captura todas las notas MIDI generadas por el análisis de biodatos y las envía en grupos vía MQTT a InfluxDB. 

**✅ NO modifica ninguna funcionalidad MIDI existente** (USB, BLE, WiFi/RTP, Serial).

---

## ⚙️ Configuración

### 1️⃣ Librerías Requeridas

**⚠️ IMPORTANTE**: Instala estas librerías ANTES de compilar:

En el Arduino IDE → Herramientas → Administrar Bibliotecas:

- **PubSubClient** (v2.8+) - Cliente MQTT por Nick O'Leary
- **ArduinoJson** (v6.x) - Por Benoit Blanchon

Si no las instalas, obtendrás errores de compilación.

### 2️⃣ Credenciales

Edita el archivo `secrets.h` con tus credenciales:

```cpp
// WiFi
#define WIFI_SSID "tu_red_wifi"
#define WIFI_PASSWORD "tu_contraseña"

// MQTT
#define MQTT_BROKER "tu_broker.com"
#define MQTT_PORT 1883
#define MQTT_USER "usuario"
#define MQTT_PASSWORD "contraseña"
```

### 3️⃣ Sensor ID

Por defecto, el ID se genera automáticamente desde la dirección MAC:
- Formato: `biodata_AABBCC`

Si prefieres un ID fijo, descomenta en `secrets.h`:
```cpp
#define SENSOR_ID_MANUAL "biodata_planta_01"
```

---

## 🚀 Uso

### Activación Automática

El buffer se activa automáticamente cuando **WiFi está ON** en el menú.

### Menú LED (mismo que antes):

1. **LED Rojo** - MIDI Scale
2. **LED Amarillo** - MIDI Channel  
3. **LED Verde** - WiFi ON/OFF ← Activa/desactiva buffer MQTT
4. **LED Azul** - Bluetooth ON/OFF

### Configuración del Buffer

En `MQTTInflux.ino`:

```cpp
#define BUFFER_SEND_INTERVAL 10000  // 10 segundos (ajustable)
#define MIDI_BUFFER_SIZE 100        // Max 100 notas en buffer
```

---

## 📊 Formato de Datos MQTT

### Topic
```
biodata/{SENSOR_ID}/midi
```

### Payload JSON
```json
{
  "sensor_id": "biodata_a1b2c3",
  "timestamp": 1234567890,
  "count": 5,
  "notes": [
    {"t": 1001, "n": 60, "v": 90, "d": 500, "c": 1},
    {"t": 1150, "n": 62, "v": 85, "d": 450, "c": 1},
    {"t": 1300, "n": 64, "v": 95, "d": 600, "c": 1}
  ]
}
```

**Campos:**
- `t` - timestamp relativo (millis)
- `n` - nota MIDI (0-127)
- `v` - velocity (0-127)
- `d` - duration (ms)
- `c` - canal MIDI (1-16)

---

## 🔍 Monitoreo Serial

Con `debugSerial = 1`, verás mensajes como:

```
=== MQTT Buffer Setup ===
Sensor ID: biodata_a1b2c3
MQTT Broker: mqt.sinfoniabiotica.xyz
Buffer interval: 10 seconds

Conectando MQTT... intento 1
✓ MQTT conectado

✓ MQTT: Enviadas 12 notas (458 bytes)
```

---

## 🛠️ Solución de Problemas

### El buffer no envía nada

✅ **Normal**: Si no se generan notas, no se envía nada (comportamiento esperado)

### MQTT no conecta

1. Verifica credenciales en `secrets.h`
2. Confirma que el broker es accesible
3. Revisa el Serial Monitor para errores

### Buffer se llena muy rápido

- Aumenta `BUFFER_SEND_INTERVAL` (ej. 5000 = 5 segundos)
- O aumenta `MIDI_BUFFER_SIZE` (ej. 150)

---

## 📝 Archivos Modificados

### ✨ Nuevos:
- `secrets.h` - Credenciales centralizadas
- `MQTTInflux.ino` - Toda la lógica de buffer/MQTT

### 📌 Modificados (cambios mínimos):
- `Biodata_Feather_ESP32_07.ino` - Include secrets.h (3 líneas)
- `Main.ino` - Setup y loop (8 líneas agregadas)
- `MIDI.ino` - Llamada a buffer (3 líneas agregadas)

**Total: 14 líneas agregadas a código existente**

---

## 🔐 Seguridad

⚠️ **IMPORTANTE**: No subas `secrets.h` a repositorios públicos.

Agrega a `.gitignore`:
```
secrets.h
```

---

## 🎵 Flujo de Datos

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

---

## 📖 Más Información

- Plan técnico completo: `00_Biotada_MolinoLab_versions/coms/2_PLAN.md`
- Configuración MQTT: `secrets.h`
- Lógica del buffer: `MQTTInflux.ino`

---

**Versión**: 1.0  
**Fecha**: Octubre 2025  
**Buffer Interval**: 10 segundos  
**Buffer Size**: 100 notas máx.

