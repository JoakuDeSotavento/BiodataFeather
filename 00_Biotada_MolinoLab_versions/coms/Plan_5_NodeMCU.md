# Plan técnico: NodeMCU v3 MQTT 5min

## Descripción breve
- Desarrollar un programa para NodeMCU v3 (ESP8266) que “haga lo mismo que `EnvironmentalData2InfluxDB` y publique por MQTT cada 5 minutos pero adaptado para un NodeMcu v3 (8266)”, omitiendo todo el soporte del sensor de lux `LTR329`.
- Se mantiene la lectura del BME688 vía `DFRobot_BME68x`, la estructura de payload JSON y el envío hacia el mismo broker MQTT definido en `secrets.h`, ajustando la cadencia a 300 000 ms.

## Archivos y ubicaciones relevantes
- `00_Biotada_MolinoLab_versions/EnvironmentalData2InfluxDB/EnvironmentalData2InfluxDB.ino`: referencia base de lógica de sensores y publicación MQTT.
- `00_Biotada_MolinoLab_versions/EnvironmentalData2InfluxDB/secrets.h`: credenciales WiFi/MQTT reutilizables.
- Nueva carpeta propuesta: `00_Biotada_MolinoLab_versions/NodeMCUv3_MQTT/`
  - `NodeMCUv3_MQTT.ino`: sketch adaptado al ESP8266.
  - `secrets.h`: copia del archivo existente (mantener política de datos sensibles).

## Cambios principales requeridos
- **Entorno ESP8266**
  - Sustituir `#include <WiFi.h>` por `#include <ESP8266WiFi.h>`.
  - Reemplazar `WiFiClientSecure` por `BearSSL::WiFiClientSecure` si se mantiene HTTPS hacia Influx; si se elimina, remover código TLS/HTTP y `HTTPClient`.
  - Ajustar inicialización de I2C con `Wire.begin(D2, D1)` (GPIO4/GPIO5 en NodeMCU) para el BME688.
- **Sensores**
  - Eliminar todas las referencias a `Adafruit_LTR329_LTR303.h`, objetos `ltr`, configuración de ganancia/tiempos y campos `visible_ir`/`infrarrojo` del JSON.
  - Conservar rutina del BME688 (`startConvert`, `update`, lecturas de temperatura/presión/humedad/gas/altitud); validar si `readCalibratedAltitude` requiere `seaLevel` opcional.
- **Temporización**
  - Cambiar `READ_INTERVAL` a `300000UL`.
  - Proteger el temporizador con `millis()` (unsigned long) y reiniciar `lastRead` al primer ciclo.
- **MQTT**
  - Mantener `PubSubClient` con `WiFiClient` (no TLS). Verificar `setBufferSize` adecuado (ESP8266 por defecto 256).
  - Reutilizar `reconnect_mqtt()` con credenciales `MQTT_USER`/`MQTT_PASSWORD`.
- **InfluxDB**
  - Si se desea conservar el envío HTTP: adaptar a `WiFiClientSecure` de ESP8266 y comprobar memoria; de lo contrario, documentar la exclusión para este puerto.

## Flujo del sketch propuesto
- `setup()`
  - Configurar `Serial` 9600 (según serigrafía de la placa).
  - Inicializar I2C y `DFRobot_BME68x_I2C bme(0x77)`.
  - Conectar WiFi con `setup_wifi()` reutilizando credenciales.
  - Generar `sensorId` (usar `SENSOR_ID` o MAC derivado).
  - Configurar cliente MQTT y `setKeepAlive(15)`.
- `loop()`
  - Llamar `mqtt.loop()`.
  - Reconectar usando `reconnect_mqtt()` si el enlace se cae.
  - Cada 300 s ejecutar `read_sensors()`.
- `read_sensors()`
  - Ejecutar ciclo de conversión BME688, obtener lecturas y registrar en `Serial`.
  - Construir `StaticJsonDocument` con claves `temperatura`, `presion`, `humedad`, `gas`, `altitud`.
  - Serializar en buffer (asegurar tamaño suficiente) y publicar con `send_mqtt()`.

## Consideraciones adicionales
- El ESP8266 tiene menos RAM que el ESP32: revisar uso de `StaticJsonDocument` y liberar buffers locales.
- Añadir `yield()` en bucles prolongados para evitar el watchdog del ESP8266.
- Validar que el BME688 reciba alimentación 3.3 V y que el pin `EN` de la placa esté alto.
- Documentar diferencias respecto a la versión ESP32 (sin LTR329 ni envío rápido) en README del nuevo directorio.

## Pruebas sugeridas
- Compilar con “NodeMCU 1.0 (ESP-12E Module)” en Arduino IDE y verificar dependencias (`ESP8266WiFi`, `PubSubClient`, `DFRobot_BME68x`, `ArduinoJson`).
- Conectar el hardware y revisar monitor serie para confirmar lecturas válidas cada 5 minutos.
- Usar un cliente MQTT (ej. `mosquitto_sub`) para validar payload JSON en `environmental/<sensorId>`.
- Desconectar/reconectar WiFi para confirmar reconexión automática del cliente MQTT.


