# Plan técnico: AZ-Envy v4.1 → MQTT

## Descripción breve
- Implementar un programa para un AZ-Envy v4.1 que “reciba cada 5 minutos los datos de todos sus sensores y los envíe por MQTT con la misma lógica y al mismo servidor que `EnvironmentalData2InfluxDB`”.
- Se reutilizarán las credenciales y parámetros ya definidos en `secrets.h`, manteniendo la política del proyecto de alojar únicamente datos sensibles en ese archivo.

## Archivos y ubicaciones relevantes
- `00_Biotada_MolinoLab_versions/EnvironmentalData2InfluxDB/EnvironmentalData2InfluxDB.ino`: referencia directa de la lógica de lectura/MQTT actual que debemos replicar.
- `00_Biotada_MolinoLab_versions/EnvironmentalData2InfluxDB/secrets.h`: contiene `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_BROKER`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASSWORD`, `MQTT_BASE_TOPIC` y `SENSOR_ID`, todos reutilizables.
- Nuevo directorio recomendado `00_Biotada_MolinoLab_versions/AZEnvy_v4_MQTT/` (misma jerarquía que otras versiones, respetando la preferencia del usuario); contendrá:
  - `AZEnvy_v4_MQTT.ino`: sketch principal.
  - (Opcional) `README.md` con notas específicas de hardware si es necesario.

## Adaptaciones de hardware y librerías
- Inventariar todos los sensores integrados en el AZ-Envy v4.1 (p.ej. temperatura/humedad, presión, VOC, luz, movimiento, etc.) consultando la hoja de datos del fabricante; registrar direcciones I2C o pines necesarios.
- Confirmar qué librerías Arduino oficiales o de terceros soportan cada sensor; documentar versiones necesarias y añadir `#include` correspondientes en el nuevo sketch.
- Determinar si el AZ-Envy requiere configuración previa de buses (Wire/SPI) o alimentación de periféricos (pines EN/3V3) y reflejarlo en `setup()`.

## Lógica principal propuesta
- `setup()`:
  - Inicializar `Serial` para depuración.
  - Configurar cada sensor del AZ-Envy siguiendo sus secuencias recomendadas (p.ej. calibración inicial de gas, fijar ganancia en sensores ópticos, establecer oversampling en métricas ambientales).
  - Llamar a `setup_wifi()` reutilizado de la referencia para conectarse a la red indicada en `secrets.h`.
  - Inicializar cliente MQTT (`PubSubClient`) apuntando a `mqtt.sinfoniabiotica.xyz` puerto `1883`; conservar `mqtt.setKeepAlive(15)`.
  - Generar `sensorId` igual que en el sketch de referencia (usar `SENSOR_ID` o derivar del MAC).
- `loop()`:
  - Vigilar `mqtt.connected()` e invocar `reconnect_mqtt()` si se pierde la sesión, replicando el flujo del archivo de referencia (usuario `biodata`, contraseña `B10d4t4?`, reintento cada 5 s).
  - Usar un temporizador basado en `millis()` con un `READ_INTERVAL = 300000` ms (5 minutos) para disparar `read_sensors()`.
  - Llamar periódicamente a `mqtt.loop()` para mantener el enlace vivo.
- `read_sensors()` (nuevo):
  - Realizar la adquisición de todos los sensores integrados. Para cada uno especificar:
    - Pasos de activación (p.ej. `startConvert()`, `delay` de estabilización, lectura de registros).
    - Conversión a unidades físicas (°C, %RH, Pa, lux, ppm, etc.), aplicando factores de escala documentados.
    - Validación de datos (p.ej. descartar lecturas `NaN`, recibir banderas de calidad).
  - Construir un `StaticJsonDocument` suficientemente grande con un campo por cada métrica reportada por el AZ-Envy.
  - Serializar el JSON y publicar en el tópico `String(MQTT_BASE_TOPIC) + "/" + sensorId` con QoS 0, igual que el sketch base.
- `send_mqtt()`, `setup_wifi()` y `reconnect_mqtt()` pueden copiar la estructura del sketch de referencia, ajustando únicamente mensajes de depuración según convenga.

## Consideraciones adicionales
- Ajustar memoria del `StaticJsonDocument` según el número de variables; documentar cálculo (bytes por entrada + overhead) para evitar fragmentación.
- Revisar si algún sensor requiere lectura menos frecuente; de ser así, cachear valores para que el envío cada 5 minutos use la última medición válida.
- Añadir mensajes `Serial` que indiquen el ciclo de 5 minutos y el payload transmitido para facilitar validaciones.
- Validar que `READ_INTERVAL` respete cambios de milisegundos (usar `unsigned long` y proteger contra overflow con resta).
- Confirmar consumo energético: si se usan modos de bajo consumo, describir cómo se retomará la conexión MQTT tras `delay` prolongados (opcional).

## Pruebas sugeridas
- Verificar conexión WiFi/MQTT desde consola serie hasta confirmar publicación exitosa en `environmental/<sensorId>`.
- Validar cada sensor individualmente (comparar con lecturas esperadas o herramientas externas).
- Monitorear el broker (por ejemplo con un cliente MQTT) para confirmar recepción cada exactamente 5 minutos.
- Probar reconexión forzando desconexión de red para asegurar que `reconnect_mqtt()` restablece la sesión.


