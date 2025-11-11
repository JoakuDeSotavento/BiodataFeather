# Plan técnico: Optimización y eficiencia del sistema Biodata MQTT

## Descripción breve
Análisis y propuesta de mejoras de eficiencia y optimización para el programa `Biodata_Feather_ESP32_10_MQTT`, con especial atención a la gestión y envío de buffers y paquetes MQTT. El objetivo es reducir consumo de memoria, evitar bloqueos, mejorar el throughput y prevenir pérdida de datos.

## Archivos y ubicaciones relevantes

### Archivos principales a modificar:
- `00_Biotada_MolinoLab_versions/Biodata_Feather_ESP32_10_MQTT/MQTTInflux.ino`: Contiene toda la lógica de buffers MQTT, serialización JSON y envío. Requiere refactorización significativa.
- `00_Biotada_MolinoLab_versions/Biodata_Feather_ESP32_10_MQTT/Main.ino`: Loop principal que gestiona llamadas a funciones MQTT. Tiene llamadas duplicadas a `flushMQTTPayload()`.
- `00_Biotada_MolinoLab_versions/Biodata_Feather_ESP32_10_MQTT/MIDI.ino`: Llama a `addNoteToBuffer()` con verificación redundante de conexión MQTT.

### Archivos de referencia:
- `00_Biotada_MolinoLab_versions/Biodata_Feather_ESP32_10_MQTT/SampleAnalysis.ino`: Genera datos que alimentan el buffer.
- `00_Biotada_MolinoLab_versions/Biodata_Feather_ESP32_10_MQTT/Biodata_Feather_ESP32_10_MQTT.ino`: Define constantes y variables globales relacionadas con MQTT.

## Problemas identificados y algoritmos de solución

### 1. Llamadas duplicadas a flushMQTTPayload()

**Problema:** En `Main.ino` líneas 140-141, se llama tanto `checkBufferTimer()` como `flushMQTTPayload()` en cada iteración del loop. `checkBufferTimer()` internamente llama a `flushMQTTPayload()` cuando el intervalo ha expirado, causando ejecuciones redundantes.

**Algoritmo de solución:**
- Eliminar la llamada directa a `flushMQTTPayload()` en `Main.ino` línea 141.
- `checkBufferTimer()` debe ser la única función responsable de disparar el envío basado en tiempo.
- `flushMQTTPayload()` solo debe llamarse desde `checkBufferTimer()` o cuando `forceSendPending` es verdadero (ya gestionado internamente).

**Archivos afectados:** `Main.ino` (eliminar línea 141).

---

### 2. Gestión ineficiente de memoria con String y DynamicJsonDocument

**Problema:** 
- `String payload` se crea en cada envío (línea 272 de `MQTTInflux.ino`), causando fragmentación de memoria heap.
- `DynamicJsonDocument` se crea dinámicamente en cada llamada a `sendBufferToInflux()` sin reutilización.
- El cálculo de capacidad JSON (líneas 221-231) es complejo y podría simplificarse o pre-calcularse.

**Algoritmo de solución:**
- Crear un buffer estático de tamaño fijo (`char mqttPayloadBuffer[MQTT_MAX_PACKET_SIZE]`) para serialización JSON.
- Pre-calcular el tamaño máximo esperado del JSON basado en `MIDI_BUFFER_SIZE` y `RAW_BLOCK_QUEUE_SIZE`.
- Usar `StaticJsonDocument` con tamaño pre-calculado en lugar de `DynamicJsonDocument`.
- Serializar directamente al buffer estático usando `serializeJson(doc, mqttPayloadBuffer, sizeof(mqttPayloadBuffer))`.
- Validar tamaño antes de construir el JSON: si `bufferIndex + rawCount` excede un umbral, dividir en múltiples mensajes o truncar.

**Archivos afectados:** `MQTTInflux.ino` (refactorizar `sendBufferToInflux()`).

---

### 3. Construcción repetitiva del topic MQTT

**Problema:** El topic `String topic = String(MQTT_BASE_TOPIC) + "/" + sensorID + "/midi"` se construye en cada envío (línea 276), causando asignaciones de memoria innecesarias.

**Algoritmo de solución:**
- Construir el topic una sola vez en `setupMQTT()` y almacenarlo en una variable global `char mqttTopic[64]` (buffer estático).
- Actualizar solo si `sensorID` cambia (no debería ocurrir en runtime normal).

**Archivos afectados:** `MQTTInflux.ino` (añadir variable global `mqttTopic`, construir en `setupMQTT()`).

---

### 4. Reconexión MQTT bloqueante con delay()

**Problema:** `reconnectMQTT()` usa `delay(retryDelay)` en línea 144, bloqueando el loop principal durante hasta 5 segundos, impidiendo procesamiento de interrupciones y actualización de LEDs.

**Algoritmo de solución:**
- Convertir `reconnectMQTT()` a máquina de estados no bloqueante.
- Añadir variables de estado: `unsigned long mqttReconnectLastAttempt`, `uint8_t mqttReconnectAttempts`, `uint8_t mqttReconnectState`.
- En lugar de `while()` con `delay()`, verificar en cada llamada si ha pasado el tiempo de retry y realizar un solo intento.
- Llamar `reconnectMQTT()` desde el loop principal sin bloquear.
- Mantener backoff exponencial pero basado en tiempo, no en `delay()`.

**Archivos afectados:** `MQTTInflux.ino` (refactorizar `reconnectMQTT()`), `Main.ino` (ajustar llamada si es necesario).

---

### 5. Pérdida de datos al descartar buffer en fallos

**Problema:** Cuando `sendBufferToInflux()` falla después de `MQTT_SEND_MAX_RETRIES`, el buffer se descarta completamente (líneas 354-361), perdiendo datos críticos.

**Algoritmo de solución:**
- Implementar buffer secundario (backup) de tamaño `MIDI_BUFFER_SIZE`.
- Si el envío falla después de todos los reintentos, copiar el contenido del buffer principal al backup antes de limpiar.
- En el próximo ciclo exitoso, intentar enviar primero el backup, luego el buffer actual.
- Añadir contador de fallos consecutivos; si excede un umbral (ej. 5), deshabilitar temporalmente el buffer y solo loggear errores.

**Archivos afectados:** `MQTTInflux.ino` (añadir `MIDIBufferEntry backupBuffer[MIDI_BUFFER_SIZE]`, `int backupIndex`, lógica de copia/restauración).

---

### 6. Verificación redundante de conexión MQTT

**Problema:** 
- En `MIDI.ino` línea 32 se verifica `mqtt.connected()` antes de `addNoteToBuffer()`.
- En `Main.ino` línea 136 se verifica nuevamente antes de `reconnectMQTT()`.
- En `sendBufferToInflux()` línea 203 se verifica otra vez.

**Algoritmo de solución:**
- `addNoteToBuffer()` no debe verificar conexión; debe aceptar datos siempre que `bufferEnabled` sea verdadero.
- La verificación de conexión debe centralizarse en `flushMQTTPayload()` o `sendBufferToInflux()`.
- Eliminar verificación en `MIDI.ino` línea 32; confiar en que el buffer se enviará cuando la conexión esté disponible.

**Archivos afectados:** `MIDI.ino` (simplificar condición línea 32), `MQTTInflux.ino` (mantener verificación centralizada).

---

### 7. Uso excesivo de millis() sin caché

**Problema:** `millis()` se llama múltiples veces en la misma función (`flushMQTTPayload()` línea 428, `sendBufferToInflux()` línea 236, `checkBufferTimer()` línea 372), y `currentMillis` ya está disponible globalmente desde `Main.ino`.

**Algoritmo de solución:**
- Pasar `currentMillis` como parámetro a funciones que lo necesiten, o usar la variable global `currentMillis` ya existente.
- Evitar llamadas redundantes a `millis()` dentro de la misma ejecución de función.

**Archivos afectados:** `MQTTInflux.ino` (refactorizar funciones para usar `currentMillis` global o parámetro).

---

### 8. Llamada a mqtt.loop() siempre activa

**Problema:** `mqtt.loop()` se llama en cada iteración del loop (línea 139) incluso cuando no hay actividad MQTT, consumiendo ciclos de CPU innecesarios.

**Algoritmo de solución:**
- Llamar `mqtt.loop()` solo cuando `bufferEnabled` es verdadero Y (`mqtt.connected()` O hay datos pendientes en buffer).
- Alternativamente, llamar con menor frecuencia usando un contador (ej. cada 100ms) en lugar de cada loop.

**Archivos afectados:** `Main.ino` (optimizar condición línea 139).

---

### 9. Falta de validación de tamaño de payload antes de serialización

**Problema:** El JSON se construye completamente antes de verificar si excede `MQTT_MAX_PACKET_SIZE`, desperdiciando tiempo y memoria si el mensaje es demasiado grande.

**Algoritmo de solución:**
- Calcular tamaño estimado antes de construir el JSON: `estimatedSize = baseMetadataSize + (bufferIndex * avgNoteSize) + (rawCount * avgRawBlockSize)`.
- Si `estimatedSize > MQTT_MAX_PACKET_SIZE * 0.9` (margen de seguridad), dividir el buffer en múltiples mensajes o truncar.
- Implementar función `splitBufferForMQTT()` que divide `midiBuffer` en chunks que caben en un solo mensaje MQTT.

**Archivos afectados:** `MQTTInflux.ino` (añadir validación pre-serialización, función de división de buffers).

---

### 10. queueRawBlock tiene lógica duplicada de verificación de cola llena

**Problema:** En `queueRawBlock()` (líneas 386-397) se verifica `rawBlockCount >= RAW_BLOCK_QUEUE_SIZE` dos veces consecutivas con lógica diferente, causando confusión y posible bug.

**Algoritmo de solución:**
- Consolidar la lógica: si la cola está llena, marcar `forceSendPending` y llamar `flushMQTTPayload()` una vez.
- Si después de intentar flush la cola sigue llena, descartar el elemento más antiguo (overwrite) y continuar.
- Simplificar a una sola verificación con manejo claro de overflow.

**Archivos afectados:** `MQTTInflux.ino` (refactorizar `queueRawBlock()` líneas 386-397).

---

### 11. Optimización de construcción JSON con campos abreviados

**Problema:** Los campos JSON usan nombres completos (`"timestamp"`, `"sensor_id"`, etc.) aumentando el tamaño del payload. Los campos de notas ya usan abreviaciones (`"t"`, `"n"`, `"v"`, `"d"`), pero los metadatos no.

**Algoritmo de solución:**
- Abreviar metadatos: `"sensor_id"` → `"sid"`, `"timestamp"` → `"ts"`, `"count"` → `"c"`, `"raw_count"` → `"rc"`.
- Mantener compatibilidad documentando el mapeo en comentarios.
- Reducirá el tamaño del payload en aproximadamente 15-20 bytes por mensaje.

**Archivos afectados:** `MQTTInflux.ino` (actualizar nombres de campos en construcción JSON).

---

### 12. Falta de canal MQTT para canal MIDI

**Problema:** El campo `midiChannel` se almacena en el buffer (línea 173) pero no se incluye en el JSON serializado (líneas 243-249), perdiendo información útil.

**Algoritmo de solución:**
- Incluir campo `"c"` (channel) en cada objeto de nota en el JSON.
- Actualizar cálculo de capacidad JSON para incluir este campo adicional.

**Archivos afectados:** `MQTTInflux.ino` (añadir `note["c"]` en línea 249, ajustar cálculo de capacidad).

---

## Consideraciones adicionales

- **Prioridad de implementación:** Los problemas 1, 2, 4 y 5 tienen mayor impacto en estabilidad y eficiencia. Los problemas 3, 7, 8 y 11 son optimizaciones menores pero fáciles de implementar.
- **Testing:** Después de cada cambio, verificar que no se pierden datos durante reconexiones WiFi/MQTT, que el consumo de memoria heap se mantiene estable, y que el throughput de envío no se degrada.
- **Compatibilidad:** Los cambios en formato JSON (problema 11) requieren actualización del consumidor (Node-RED) si se implementan. Considerar mantener ambos formatos temporalmente con flag de compatibilidad.

