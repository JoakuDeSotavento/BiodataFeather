# Plan Técnico: Registro de datos crudos y metadatos vía MQTT

## Descripción
Implementar la primera mejora de `mejorasBiodata.md`: registrar cada bloque de muestras eléctricas antes de mapearlo a notas MIDI, enviando únicamente los valores crudos (las 9 muestras capturadas) y el `threshold` vigente. No se enviarán otros datos de configuración ni se registrarán eventos. Aprovecharemos la infraestructura ya existente en `MQTTInflux.ino` para serializar y publicar un único mensaje MQTT que incluya notas MIDI y, cuando existan, bloques crudos.

## Archivos y Funciones a Modificar

- `Biodata_Feather_ESP32_07/SampleAnalysis.ino`
  - En `sample()` y `analyzeSample()` capturar el bloque de 9 muestras (`samples[1..analysize]`) antes del procesamiento.
  - Calcular max/min/media/desviación/delta y enviar el paquete a `queueRawBlock()` (nueva función declarada en `MQTTInflux.ino`), incluyendo el valor actual de `threshold`.

- `Biodata_Feather_ESP32_07/Main.ino`
  - Incluir el prototipo de `queueRawBlock()`.
  - En el `loop()`, después de `checkBufferTimer()`, llamar a `flushMQTTPayload()` para disparar el envío cuando haya datos crudos pendientes, manteniendo la lógica de reconexión existente.

- No es necesario modificar `Biodata_Feather_ESP32_07/MIDI.ino` (no habrá registro de eventos).

- `Biodata_Feather_ESP32_07/MQTTInflux.ino`
  - Añadir la estructura `RawSampleBlock` y su cola circular.
  - Implementar `queueRawBlock()` y `flushMQTTPayload()` dentro del mismo archivo para reutilizar `mqtt`, `sensorID`, `BUFFER_SEND_INTERVAL` y `sendBufferToInflux()`.
  - Modificar `sendBufferToInflux()` para:
    - Comprobar si hay notas o bloques crudos; si ambos están vacíos, salir.
    - Construir un único `DynamicJsonDocument` que incluya:
      - `notes`: array existente (igual que ahora).
      - `raw_blocks`: array opcional con los bloques capturados (muestras, estadísticas básicas y `threshold`).
    - Mantener el topic actual (`{MQTT_BASE_TOPIC}/{sensorID}/midi`) para no cambiar flujos, añadiendo simplemente más campos.
    - Vaciar las colas después de un publish exitoso.
  - Ajustar `checkBufferTimer()` para que invoque `sendBufferToInflux()` solo a través de `flushMQTTPayload()` (de modo que se respete el intervalo pero también se puedan forzar envíos cuando se llene un buffer).

- `Biodata_Feather_ESP32_07/README_MQTT_BUFFER.md`
  - Documentar el nuevo esquema del payload JSON (campo `raw_blocks`) para Node-RED.

- `secrets.h`
  - Verificar que no es necesario añadir topics adicionales; si se requiere separar topic, definir `MQTT_TOPIC_RAW` pero idealmente mantener el mismo topic para un solo mensaje.

## Algoritmo Paso a Paso

1. **Captura de Bloque**
   - Cuando `sampleIndex >= samplesize`:
     - Duplicar las muestras en un array local (9 elementos).
     - Calcular estadísticos (max, min, avg, std, delta) usando la misma lógica que `analyzeSample()` para garantizar consistencia.
    - Tomar el valor actual de `threshold`.
    - Llamar a `queueRawBlock(timestamp, stats, samples, threshold)`.

2. **Cola compartida**
   - La función `queueRawBlock()` residirá en `MQTTInflux.ino` junto a `midiBuffer` para compartir lógica de reconexión y aprovechar el `DynamicJsonDocument` que ya se construye en `sendBufferToInflux()`.
   - Definir límite `RAW_BLOCK_QUEUE_SIZE` y política FIFO: descartar el bloque más antiguo si la cola se llena.

3. **Serialización y envío**
   - `flushMQTTPayload()` comprobará si se alcanzó `BUFFER_SEND_INTERVAL` o si algún buffer llegó a su límite (forzando envío inmediato).
   - Internamente llamará a `sendBufferToInflux()` que:
     - Usa el `mqtt` ya configurado, `sensorID`, `MQTT_MAX_PACKET_SIZE`.
     - Inserta campos nuevos en el mismo JSON antes de serializar:
       ```cpp
       if (rawBlockCount > 0) {
         JsonArray raw = doc.createNestedArray("raw_blocks");
         // Añadir bloques con samples[], stats{} y threshold
       }
       ```
     - Envía una vez por intervalo; si no hay notas pero sí crudos, se publica igualmente (así Node-RED recibe datos científicos aunque no haya notas).

4. **Integración con Node-RED**
   - Actualizar documentación para reflejar el nuevo JSON: `notes` + `raw_blocks` en un único mensaje por intervalo.
   - Node-RED seguirá usando el mismo topic; solo requiere ajustar el flow para extraer el array adicional.

## Consideraciones

- Reutilizamos totalmente `mqtt`, `sensorID`, `sendBufferToInflux()`, `checkBufferTimer()` y la infraestructura existente, evitando archivos nuevos.
- Mantener `DynamicJsonDocument` dentro de los límites de `MQTT_MAX_PACKET_SIZE`; dimensionar capacidad extra para `raw_blocks` (9 enteros + estadísticas + threshold).
- Añadir `ENABLE_RAW_LOGGING` como `#define` opcional en `MQTTInflux.ino` para activar/desactivar la cola y minimizar consumo cuando no se necesite.
- Garantizar que las ISR (`sample()`) solo copian datos mínimos; el cálculo pesado y la cola se hacen fuera de la interrupción (en `analyzeSample()`).
- Mantener compatibilidad con el envío de notas MIDI: si se deshabilita el buffer MIDI actual, los bloques crudos pueden seguir usándose si se desea.

