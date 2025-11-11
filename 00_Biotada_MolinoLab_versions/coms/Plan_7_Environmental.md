# Plan 7: Integración de Sensores Ambientales

## Descripción
Integrar la funcionalidad de lectura y envío de datos ambientales desde `EnvironmentalData2InfluxDB` (código en producción) al proyecto `Biodata_Feather_ESP32_11_Environmental`. **IMPORTANTE: El código fuente de `EnvironmentalData2InfluxDB` está en producción y funciona correctamente con ESP32-S3. Se debe COPIAR Y PEGAR el código necesario haciendo solo los cambios mínimos para integrarlo.**

## Archivos a modificar/crear

### Archivos nuevos:
- `Environmental.ino` - Nuevo archivo creado copiando y adaptando código de `EnvironmentalData2InfluxDB.ino`

### Archivos a modificar:
- `Biodata_Feather_ESP32_10_MQTT.ino` - Agregar declaraciones de funciones ambientales
- `Main.ino` - Agregar inicialización de sensores ambientales en setup() y llamadas en loop()
- `secrets.h` - Agregar configuración de topic MQTT #define MQTT_ENV_TOPIC "env" para datos ambientales

## Estrategia de integración: Copiar y adaptar código existente

### Código fuente a copiar:
- **Origen**: `EnvironmentalData2InfluxDB.ino` (líneas 1-203)
- **Estado**: Código en producción funcionando correctamente con ESP32-S3
- **Enfoque**: Copiar líneas específicas y hacer cambios mínimos

### 1. Inicialización de sensores (Environmental.ino)
**Copiar de `EnvironmentalData2InfluxDB.ino`:**
- Líneas 7-15: Includes (copiar tal cual, excepto WiFi que ya está en el proyecto)
- Líneas 17-19: Declaración de sensores (copiar tal cual)
- Líneas 27-29: Variables de timing (copiar tal cual, renombrar `lastRead` → `lastEnvironmentalRead`)
- Líneas 31-32: Sea level (copiar tal cual si se usa CALIBRATE_PRESSURE)
- Líneas 47-62: Inicialización de sensores en setup() → mover a `setupEnvironmentalSensors()`
- Líneas 77-81: Configuración de sensores → mover a `setupEnvironmentalSensors()`

**Cambios mínimos:**
- Eliminar includes de WiFi/WiFiClientSecure/HTTPClient (ya existen en el proyecto)
- Eliminar creación de cliente MQTT (usar `extern PubSubClient mqtt` existente)
- Eliminar `setup_wifi()` y `reconnect_mqtt()` (ya existen en el proyecto)
- Cambiar `sensorId` por `sensorID` (usar variable externa del proyecto)
- Cambiar `lastRead` por `lastEnvironmentalRead`
- Cambiar `READ_INTERVAL` por `ENVIRONMENTAL_READ_INTERVAL`
- Envolver inicialización en función `setupEnvironmentalSensors()`

### 2. Lectura de sensores (función `readEnvironmentalSensors()`)
**Copiar de `EnvironmentalData2InfluxDB.ino`:**
- Líneas 125-172: Función `read_sensors()` completa → renombrar a `readEnvironmentalSensors()`

**Cambios mínimos:**
- Renombrar función de `read_sensors()` a `readEnvironmentalSensors()`
- Cambiar `sensorId` por `sensorID` (variable externa)
- Cambiar topic MQTT de `String(MQTT_BASE_TOPIC) + "/" + sensorId` a `String(MQTT_BASE_TOPIC) + "/environmental/" + sensorID`
- Eliminar llamada a `send_mqtt()` y mover lógica directamente (o crear función interna)
- Usar `mqtt.publish()` directamente en lugar de `send_mqtt()`

### 3. Envío vía MQTT (función `sendEnvironmentalData()`)
**Copiar de `EnvironmentalData2InfluxDB.ino`:**
- Líneas 156-171: Creación de JSON y envío MQTT
- Línea 174-176: Función `send_mqtt()` → adaptar a `sendEnvironmentalData()`

**Cambios mínimos:**
- Extraer creación de JSON a función `sendEnvironmentalData()` con parámetros
- Cambiar topic a `{MQTT_BASE_TOPIC}/environmental/{sensorID}`
- Usar `mqtt.publish()` directamente (cliente MQTT ya existe)

### 4. Integración en el loop principal
**Copiar de `EnvironmentalData2InfluxDB.ino`:**
- Líneas 118-121: Lógica de timing en loop() → adaptar a `checkEnvironmentalTimer()`

**Cambios mínimos:**
- Crear función `checkEnvironmentalTimer()` que encapsule la lógica de timing
- Usar `currentMillis` global en lugar de `millis()` directo
- Llamar desde `Main.ino` en el loop principal dentro del bloque `if(bufferEnabled)`

## Algoritmo de timing (copiado de EnvironmentalData2InfluxDB.ino)

```
En checkEnvironmentalTimer():
  Si (currentMillis - lastEnvironmentalRead >= ENVIRONMENTAL_READ_INTERVAL):
    lastEnvironmentalRead = currentMillis
    readEnvironmentalSensors()  // Esta función ya incluye el envío MQTT
```

## Mapeo de código: Líneas específicas a copiar

### De EnvironmentalData2InfluxDB.ino a Environmental.ino:

| Líneas origen | Contenido | Cambios necesarios |
|---------------|-----------|-------------------|
| 7-11 | Includes sensores | Copiar tal cual |
| 17-19 | Declaración sensores | Copiar tal cual |
| 27-29 | Timing variables | Copiar, renombrar variables |
| 47-62 | Init LTR329 + BME688 | Mover a función setupEnvironmentalSensors() |
| 77-81 | Config sensores | Mover a función setupEnvironmentalSensors() |
| 125-172 | read_sensors() | Renombrar función, cambiar sensorId→sensorID, topic |
| 156-171 | JSON + MQTT send | Extraer a sendEnvironmentalData(), cambiar topic |
| 118-121 | Loop timing | Adaptar a checkEnvironmentalTimer() |

## Consideraciones técnicas

- **NO reescribir código**: Copiar líneas específicas del código en producción
- **Cambios mínimos**: Solo adaptar nombres de variables y funciones para integración
- Reutilizar el cliente MQTT existente (`mqtt` de MQTTInflux.ino) - NO crear nuevo cliente
- Reutilizar WiFi existente - NO incluir setup_wifi() ni reconnect_mqtt()
- Usar `sensorID` existente del proyecto (variable externa)
- Usar `currentMillis` global en lugar de `millis()` directo
- No interferir con el funcionamiento MIDI existente
- Los sensores ambientales funcionan independientemente de los datos MIDI
- Mantener compatibilidad con la estructura modular existente

---

## ⚠️ CORRECCIONES CRÍTICAS REQUERIDAS (Basadas en análisis del código funcional)

**NOTA**: Después de implementar el plan, se encontraron problemas. Las siguientes correcciones son CRÍTICAS y están basadas en el código funcional `EnvironmentalData2InfluxDB.ino`:

### ✅ Corrección 1: Delay después de Serial.begin() - CRÍTICO [IMPLEMENTADA]

**Problema**: El código funcional tiene `delay(1000)` después de `Serial.begin(115200)` antes de inicializar sensores. Este delay es necesario para estabilización del ESP32.

**Solución**: Agregar en `Main.ino` línea 6:
```cpp
if (debugSerial || rawSerial) {
  Serial.begin(115200);
  delay(1000); // CRÍTICO: Estabilización antes de I2C
}
```

**Estado**: ✅ IMPLEMENTADA en `Main.ino` líneas 6-9

### ✅ Corrección 2: Orden de inicialización - CRÍTICO [IMPLEMENTADA]

**Problema**: En el código funcional, los sensores se inicializan **ANTES** de WiFi. En la implementación actual se inicializan después.

**Solución**: En `Main.ino`, mover `setupEnvironmentalSensors()` ANTES de `setupWifi()`:
```cpp
if (serialMIDI) setupSerialMIDI();

// Sensores Ambientales: Inicializar ANTES de WiFi (como código funcional)
if (wifiMIDI) {
  setupEnvironmentalSensors(); // ← MOVER AQUÍ
  setupWifi();
  setupMQTT();
  bufferEnabled = true;
}
```

**Estado**: ✅ IMPLEMENTADA en `Main.ino` líneas 105-112

### ✅ Corrección 3: Orden de configuración del LTR329 - IMPORTANTE [IMPLEMENTADA]

**Problema**: En el código funcional, la configuración del LTR329 (setGain, setIntegrationTime, setMeasurementRate) se hace **DESPUÉS** de inicializar el BME688.

**Solución**: En `Environmental.ino`, mover las líneas 60-63 (configuración LTR329) para que estén después de la línea 111 (después de `bme.setGasHeater()`).

**Orden correcto en `setupEnvironmentalSensors()`**:
1. Inicializar LTR329 (`ltr.begin()`)
2. Inicializar BME688 (`bme.begin()`)
3. Configurar BME688 (`bme.setGasHeater()`)
4. **Configurar LTR329** (`ltr.setGain()`, `ltr.setIntegrationTime()`, `ltr.setMeasurementRate()`)

**Estado**: ✅ IMPLEMENTADA en `Environmental.ino` líneas 108-111

### Corrección 4: NO agregar Wire.begin()

**IMPORTANTE**: El código funcional **NO tiene** `Wire.begin()` explícito. El ESP32 inicializa I2C automáticamente. **NO agregar** `Wire.begin()` a menos que sea absolutamente necesario.

**Estado**: ✅ VERIFICADO - No se agregó `Wire.begin()` (correcto)

---

## Referencias

- Código funcional: `Environmental/EnvironmentalData2InfluxDB/EnvironmentalData2InfluxDB.ino`
- Análisis de errores: `Biodata_Feather_ESP32_11_Environmental/ERRORES_Y_SOLUCIONES.md`
