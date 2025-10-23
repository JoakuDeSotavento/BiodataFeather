# Plan Técnico: Envío de Notas MIDI a InfluxDB vía MQTT con Buffering

## Descripción

Implementar un sistema de buffering que almacene las notas MIDI generadas por el análisis de biodatos durante varios segundos y las envíe de forma agrupada a InfluxDB a través de MQTT. Esto reduce significativamente el tráfico de red en comparación con envíos individuales por cada nota, manteniendo la funcionalidad MIDI existente intacta.

## ⚠️ PRINCIPIO FUNDAMENTAL: CAMBIOS MÍNIMOS

**IMPORTANTE:** Este plan se limita ESTRICTAMENTE a:
- ✅ Agregar buffer de notas MIDI
- ✅ Crear nuevo archivo MQTTInflux.ino con toda la lógica nueva
- ✅ Agregar archivo secrets.h con credenciales MQTT/InfluxDB
- ✅ Insertar llamadas mínimas en puntos estratégicos (setup, loop, setNote)
- ❌ NO modificar ninguna funcionalidad MIDI existente (USB, BLE, Serial, WiFi/RTP)
- ❌ NO alterar el flujo de análisis de muestras ni generación de notas
- ❌ NO cambiar comportamiento de LEDs, botones o menús existentes
- ❌ NO reorganizar código existente

Todo el código nuevo debe ser **aditivo y modular**, pudiendo deshabilitarse completamente sin afectar el sistema original.

## Opciones de Buffering Propuestas

### Opción 1: Buffer de 5 segundos (Recomendada)
- Intervalo fijo de 5 segundos
- ~5-15 notas por envío típico
- Balance óptimo entre latencia y eficiencia

### Opción 2: Buffer de 15 segundos
- Intervalo fijo de 15 segundos
- ~15-45 notas por envío
- Reduce más el tráfico de red

### Opción 3: Buffer de 30 segundos
- Intervalo fijo de 30 segundos
- ~30-90 notas por envío
- Mínimo tráfico, ideal para logging histórico

### Opción 4: Buffer Híbrido
- 10 segundos O 25 notas (lo que ocurra primero)
- Adaptativo a la actividad de la planta
- Mayor complejidad de implementación

## Archivos a Modificar

### 1. `Biodata_Feather_ESP32_07.ino` (archivo principal)
**Cambios necesarios:**
- Agregar includes para WiFiClient, PubSubClient y ArduinoJson
- Agregar variables de configuración para MQTT e InfluxDB
- Declarar estructura de buffer para notas MIDI
- Añadir variables de temporización para el buffer
- Incluir archivo `secrets.h` (a crear)
- Declarar cliente MQTT y funciones de conexión

**Nuevas variables globales:**
```cpp
// Buffer de notas MIDI
#define MIDI_BUFFER_SIZE 100
#define BUFFER_SEND_INTERVAL 5000  // 5 segundos (configurable)

struct MIDIBufferEntry {
  unsigned long timestamp;
  byte note;
  byte velocity;
  int duration;
  byte midiChannel;
};

MIDIBufferEntry midiBuffer[MIDI_BUFFER_SIZE];
int bufferIndex = 0;
unsigned long lastBufferSend = 0;
bool bufferEnabled = false;  // Control on/off via button menu
```

### 2. Nuevo archivo: `MQTTInflux.ino`
**Propósito:** Manejar toda la lógica de MQTT e InfluxDB

**Funciones a implementar:**

#### `setupMQTT()`
- Inicializar cliente MQTT
- Configurar servidor y puerto desde secrets.h
- Establecer callback (si es necesario para futuros mensajes entrantes)

#### `reconnectMQTT()`
- Verificar conexión MQTT
- Reintentar conexión con credenciales desde secrets.h
- Implementar backoff exponencial (delays: 1s, 2s, 4s, máx 5s)
- Máximo 3 reintentos antes de abandonar

#### `addNoteToBuffer(byte note, byte velocity, int duration, byte channel)`
- Verificar si hay espacio en el buffer
- Si está lleno, forzar envío inmediato (llamar a `sendBufferToInflux()`)
- Agregar nueva entrada con timestamp actual (millis())
- Incrementar bufferIndex

#### `sendBufferToInflux()`
- **Si bufferIndex == 0, retornar inmediatamente (buffer vacío - OPCIÓN A: No enviar nada)**
- Construir payload JSON con formato:
  ```json
  {
    "sensor_id": "biodata_xxx",
    "timestamp": millis(),
    "notes": [
      {"t": 1234, "note": 60, "vel": 90, "dur": 500, "ch": 1},
      {"t": 1456, "note": 62, "vel": 85, "dur": 450, "ch": 1}
    ]
  }
  ```
- Publicar en topic: `{MQTT_BASE_TOPIC}/{SENSOR_ID}/midi`
- Si publish exitoso, resetear bufferIndex a 0
- Si falla, mantener buffer y registrar error en Serial

#### `checkBufferTimer()`
- Verificar si han pasado BUFFER_SEND_INTERVAL miliseconds desde lastBufferSend
- Si es momento de enviar, llamar a `sendBufferToInflux()`
- Actualizar lastBufferSend

### 3. `MIDI.ino`
**Función a modificar:** `setNote()`

**Cambios:**
- Después de la sección que envía MIDI (`//*************`), agregar:
  ```cpp
  // Agregar a buffer MQTT si está habilitado
  if(bufferEnabled && mqtt.connected()) {
    addNoteToBuffer(value, velocity, duration, notechannel);
  }
  ```

### 4. `Main.ino`
**Función a modificar:** `setup()`

**Cambios:**
- Después de `setupWifi()`, agregar:
  ```cpp
  if(wifiMIDI) {
    setupMQTT();
    bufferEnabled = true;  // Habilitar buffer si WiFi está activo
  }
  ```

**Función a modificar:** `loop()`

**Cambios:**
- Después de `checkControl()`, agregar:
  ```cpp
  // Gestionar MQTT y buffer
  if(bufferEnabled) {
    if(!mqtt.connected()) {
      reconnectMQTT();
    }
    mqtt.loop();
    checkBufferTimer();
  }
  ```

### 5. Nuevo archivo: `secrets.h`
**Propósito:** Almacenar credenciales y configuraciones sensibles

**Contenido:**
```cpp
#pragma once
// WiFi (ya existentes en código principal, centralizar aquí)
#define WIFI_SSID "MolinoLab"
#define WIFI_PASSWORD "hacktheworld"

// MQTT
#define MQTT_BROKER "mqt.sinfoniabiotica.xyz"
#define MQTT_PORT 1883
#define MQTT_USER "biodata"
#define MQTT_PASSWORD "b10d4t4?"
#define MQTT_BASE_TOPIC "biodata"
#define SENSOR_ID "biodata_esp32_001"  // Usar MAC o ID único

// InfluxDB (para referencia, el broker MQTT se encarga del envío)
#define INFLUX_URL "https://db.sinfoniabiotica.xyz:443/api/v2/write?org=MolinoLab&bucket=biodata&precision=ms"
#define INFLUX_TOKEN "RUWzp6k07mf9lsmmy1UVYCEPgmuU7FR1E6aJKMMM13VSqo24U2J2QBc0-zbjzcdPOLyDHJGXLHY-AQC-MhoLdw=="
```

**Nota:** Considerar usar el MAC address para generar SENSOR_ID único automáticamente

### 6. `checkButton()` en archivo principal
**Modificación opcional:** Agregar menú para activar/desactivar buffer MQTT

**Cambios:**
- Agregar opción de menú 4 (actualmente comentada para batería)
- LED blanco para indicar estado buffer (encendido = activo)
- Toggle on/off del buffer vía knob

## Algoritmo de Buffering Detallado

### Paso 1: Inicialización
1. En `setup()`, si WiFi está activo, llamar a `setupMQTT()`
2. Establecer `bufferEnabled = true`
3. Inicializar `bufferIndex = 0`
4. Guardar timestamp inicial en `lastBufferSend = millis()`

### Paso 2: Captura de Notas
1. Cuando `setNote()` se ejecuta (nota MIDI generada)
2. Verificar `if(bufferEnabled && mqtt.connected())`
3. Llamar a `addNoteToBuffer()` con parámetros de la nota
4. Si buffer está lleno, forzar envío inmediato

### Paso 3: Envío Periódico
1. En cada iteración de `loop()`, llamar a `checkBufferTimer()`
2. Calcular: `millis() - lastBufferSend >= BUFFER_SEND_INTERVAL`
3. Si es tiempo de enviar:
   - Llamar a `sendBufferToInflux()`
   - Actualizar `lastBufferSend = millis()`

### Paso 4: Construcción de Payload JSON
1. Crear JsonDocument con capacidad adecuada (base + 80 bytes por nota)
2. Agregar metadata: sensor_id, timestamp del envío
3. Crear array "notes" con todas las entradas del buffer
4. Para cada nota: timestamp relativo, note, velocity, duration, channel
5. Serializar a String

### Paso 5: Publicación MQTT
1. Construir topic: `{MQTT_BASE_TOPIC}/{SENSOR_ID}/midi`
2. Publicar payload con `mqtt.publish(topic, payload, false)` (QoS 0)
3. Si exitoso, limpiar buffer (`bufferIndex = 0`)
4. Si falla, mantener buffer para próximo intento

### Paso 6: Manejo de Buffer Vacío (OPCIÓN A)
1. En `checkBufferTimer()`, cuando llega el momento de enviar
2. Verificar `if (bufferIndex == 0)`, simplemente retornar
3. **No se envía ningún mensaje MQTT**
4. **No se genera tráfico de red innecesario**
5. El sistema continúa normalmente, esperando al siguiente intervalo

### Paso 7: Manejo de Desconexión
1. Si MQTT se desconecta, mantener notas en buffer
2. Al reconectar, enviar buffer acumulado
3. Si buffer se llena durante desconexión, descartar notas más antiguas (FIFO)

## Dependencias Requeridas

### Librerías a incluir (agregar al proyecto):
1. **PubSubClient** (v2.8+) - Cliente MQTT para Arduino
2. **ArduinoJson** (v6.x) - Construcción de payloads JSON eficientes

### Librerías ya presentes:
- WiFi.h (ESP32)
- WiFiClient.h (ESP32)

## Consideraciones de Memoria

**RAM estimada:**
- Buffer: 100 entradas × 10 bytes = 1,000 bytes
- JsonDocument: ~2,000 bytes (buffer completo)
- Cliente MQTT: ~512 bytes
- **Total adicional: ~3.5 KB**

**ESP32-S3 disponible:** >300 KB RAM → Suficiente margen

## Formato de Datos InfluxDB

El servidor MQTT (Node-RED u otro) recibirá el JSON y lo transformará a formato InfluxDB Line Protocol:

```
biodata,sensor=biodata_esp32_001,channel=1 note=60i,velocity=90i,duration=500i 1234567890123
biodata,sensor=biodata_esp32_001,channel=1 note=62i,velocity=85i,duration=450i 1234567891456
```

**Tags:** sensor, channel  
**Fields:** note, velocity, duration  
**Timestamp:** Unix milliseconds

## Configuración Recomendada Inicial

- `BUFFER_SEND_INTERVAL = 5000` (5 segundos)
- `MIDI_BUFFER_SIZE = 100` (suficiente para ~20-30 segundos de alta actividad)
- QoS MQTT = 0 (fire and forget, menor overhead)
- Formato JSON compacto (sin espacios, claves cortas)

## Pruebas Necesarias

1. **Buffer básico:** Verificar que notas se acumulan correctamente
2. **Envío periódico:** Confirmar envíos cada 5 segundos
3. **Buffer lleno:** Probar forzado de envío cuando se alcanza límite
4. **Desconexión WiFi:** Verificar manejo de pérdida de conexión
5. **Reconexión MQTT:** Confirmar que buffer se envía al reconectar
6. **Carga alta:** Generar muchas notas rápidamente (>20/segundo)
7. **Validación JSON:** Verificar formato en broker MQTT
8. **Throughput:** Medir tiempo de envío y procesamiento

## Notas de Implementación

- La funcionalidad MIDI existente (USB, BLE, Serial, WiFi/RTP) **NO se modifica**
- El buffer es **adicional** al flujo MIDI normal
- Se puede deshabilitar completamente sin afectar MIDI tradicional
- Usar `debugSerial` existente para logs de MQTT
- **Buffer vacío = No enviar nada (OPCIÓN A seleccionada)**

## Resumen de Modificaciones Mínimas

### Archivos NUEVOS (crear):
1. **`MQTTInflux.ino`** - Toda la lógica de buffer y MQTT (~150 líneas)
2. **`secrets.h`** - Solo credenciales MQTT/InfluxDB (~20 líneas)

### Archivos MODIFICADOS (cambios mínimos):
1. **`Biodata_Feather_ESP32_07.ino`** (archivo principal):
   - Agregar includes (3 líneas)
   - Agregar variables globales del buffer (15 líneas)
   - Todo al inicio, no toca código existente

2. **`Main.ino`**:
   - En `setup()`: Agregar 3 líneas después de `setupWifi()`
   - En `loop()`: Agregar 5 líneas después de `checkControl()`
   - Total: **8 líneas insertadas**

3. **`MIDI.ino`**:
   - En `setNote()`: Agregar **3 líneas** después del bloque `//*************`
   - Solo llama a `addNoteToBuffer()` si está habilitado

### Total de cambios en código existente:
- **11 líneas insertadas** en archivos existentes
- **0 líneas modificadas** de código existente
- **0 funciones existentes alteradas**
- **~170 líneas nuevas** en archivos nuevos (aisladas y modulares)

### Lo que NO se toca:
- ❌ Análisis de muestras (`analyzeSample()`)
- ❌ Generación de notas MIDI
- ❌ Sistema de LEDs y faders
- ❌ Menús de botón existentes
- ❌ Configuración de BLE
- ❌ Configuración de WiFi/RTP
- ❌ Sistema de escalas y notas
- ❌ Control de velocidad y duración
- ❌ Ninguna otra funcionalidad existente

