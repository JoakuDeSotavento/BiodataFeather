// ============================================================================
// MQTT INFLUX BUFFER - Sistema de buffering de notas MIDI para envío a InfluxDB
// ============================================================================
// Este archivo contiene toda la lógica para almacenar notas MIDI en un buffer
// y enviarlas periódicamente vía MQTT a InfluxDB.
// No modifica ninguna funcionalidad MIDI existente.
// ============================================================================
// Nota: Los includes y secrets.h están en el archivo principal
#include <cstring>

// Configuración del buffer
#ifndef ENABLE_RAW_LOGGING
#define ENABLE_RAW_LOGGING 1
#endif

#define MIDI_BUFFER_SIZE 100              // Tamaño máximo del buffer
#define BUFFER_SEND_INTERVAL 10000        // 10 segundos entre envíos
#define MQTT_SEND_MAX_RETRIES 1           // Número de reintentos adicionales antes de descartar

#if ENABLE_RAW_LOGGING
#define RAW_BLOCK_QUEUE_SIZE 6

struct RawSampleBlock {
  unsigned long timestamp;
  unsigned long maximum;
  unsigned long minimum;
  unsigned long average;
  float stddev;
  unsigned long delta;
  float threshold;
};

static RawSampleBlock rawBlockQueue[RAW_BLOCK_QUEUE_SIZE];
static size_t rawBlockWriteIndex = 0;
static size_t rawBlockReadIndex = 0;
static size_t rawBlockCount = 0;
#endif

// Estructura para almacenar notas MIDI en el buffer
struct MIDIBufferEntry {
  unsigned long timestamp;   // Timestamp relativo (millis)
  byte note;                // Número de nota MIDI (0-127)
  byte velocity;            // Velocidad de la nota (0-127)
  int duration;             // Duración de la nota en ms
  byte midiChannel;         // Canal MIDI (1-16)
};

// Variables globales del buffer
MIDIBufferEntry midiBuffer[MIDI_BUFFER_SIZE];
int bufferIndex = 0;
unsigned long lastBufferSend = 0;
bool bufferEnabled = false;
bool forceSendPending = false;
// static bool wifiDisconnectedLogged = false;

// Cliente MQTT
WiFiClient mqttWifiClient;
PubSubClient mqtt(mqttWifiClient);

// Tamaño máximo del buffer MQTT (por defecto es 256, aumentamos a 1024)
#define MQTT_MAX_PACKET_SIZE 1024

// Sensor ID (generado desde MAC)
String sensorID = "";

void flushMQTTPayload();

// ============================================================================
// SETUP MQTT - Inicialización del cliente MQTT
// ============================================================================
void setupMQTT() {
  // Generar Sensor ID desde MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  
  #ifdef SENSOR_ID_MANUAL
    sensorID = SENSOR_ID_MANUAL;
  #else
    int uniq = 0;
    for (int i = 0; i < 6; i++) {
      uniq += mac[i];
    }
    sensorID = "biodata_";
    sensorID += String(uniq);
  #endif
  
  if (debugSerial) {
    Serial.println("=== MQTT Buffer Setup ===");
    Serial.print("Sensor ID: ");
    Serial.println(sensorID);
    Serial.print("MQTT Broker: ");
    Serial.println(MQTT_BROKER);
    Serial.print("Buffer interval: ");
    Serial.print(BUFFER_SEND_INTERVAL / 1000);
    Serial.println(" seconds");
    Serial.print("Max MQTT packet: ");
    Serial.print(MQTT_MAX_PACKET_SIZE);
    Serial.println(" bytes");
  }
  
  // Configurar servidor MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setKeepAlive(15);
  mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE);  // Aumentar tamaño máximo de mensajes
  
  // Intentar conectar
  reconnectMQTT();
  
  // Inicializar timestamp de último envío
  lastBufferSend = millis();
}

// ============================================================================
// RECONNECT MQTT - Reconexión al broker MQTT con backoff exponencial
// ============================================================================
void reconnectMQTT() {
  // Solo intentar reconectar si WiFi está conectado
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  int retryCount = 0;
  int retryDelay = 1000; // Empezar con 1 segundo
  
  while (!mqtt.connected() && retryCount < 3) {
    if (debugSerial) {
      Serial.print("Conectando MQTT... intento ");
      Serial.println(retryCount + 1);
    }
    
    // Intentar conexión con credenciales
    if (mqtt.connect(sensorID.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      if (debugSerial) {
        Serial.println("✓ MQTT conectado");
      }
      return;
    } else {
      if (debugSerial) {
        Serial.print("✗ MQTT falló, rc=");
        Serial.print(mqtt.state());
        Serial.print(" - Reintentando en ");
        Serial.print(retryDelay / 1000);
        Serial.println("s");
      }
      
      delay(retryDelay);
      retryCount++;
      retryDelay = min(retryDelay * 2, 5000); // Backoff exponencial, máx 5s
    }
  }
  
  if (!mqtt.connected() && debugSerial) {
    Serial.println("✗ MQTT: No se pudo conectar después de 3 intentos");
  }
}

// ============================================================================
// ADD NOTE TO BUFFER - Agregar una nota MIDI al buffer
// ============================================================================
void addNoteToBuffer(byte note, byte velocity, int duration, byte channel) {
  // Verificar si el buffer está lleno
  if (bufferIndex >= MIDI_BUFFER_SIZE) {
    if (debugSerial) {
      Serial.println("⚠ Buffer lleno - Forzando envío inmediato");
    }
    forceSendPending = true;
    flushMQTTPayload();
  }
  
  // Agregar nota al buffer
  midiBuffer[bufferIndex].timestamp = millis();
  midiBuffer[bufferIndex].note = note;
  midiBuffer[bufferIndex].velocity = velocity;
  midiBuffer[bufferIndex].duration = duration;
  midiBuffer[bufferIndex].midiChannel = channel;
  
  bufferIndex++;
}

// ============================================================================
// SEND BUFFER TO INFLUX - Enviar buffer de notas y bloques crudos vía MQTT
// ============================================================================
bool sendBufferToInflux() {
#if ENABLE_RAW_LOGGING
  const size_t rawCount = rawBlockCount;
#else
  const size_t rawCount = 0;
#endif

  if (bufferIndex == 0 && rawCount == 0) {
    return false;
  }

  // if (WiFi.status() != WL_CONNECTED) {
  //   if (debugSerial && !wifiDisconnectedLogged) {
  //     Serial.println("⚠ MQTT: WiFi no conectada; buffers pendientes en espera");
  //   }
  //   wifiDisconnectedLogged = true;
  //   return false;
  // } else {
  //   wifiDisconnectedLogged = false;
  // }
  
  // Verificar conexión MQTT y reconectar si es necesario
  if (!mqtt.connected()) {
    if (debugSerial) {
      Serial.print("⚠ MQTT desconectado (estado: ");
      Serial.print(mqtt.state());
      Serial.println(") - Intentando reconectar...");
    }
    reconnectMQTT();
    
    // Verificar si la reconexión fue exitosa
    if (!mqtt.connected()) {
      if (debugSerial) {
        Serial.println("✗ No se pudo reconectar - Buffer retenido");
      }
      return false;
    }
  }
  
  // Construir payload JSON
  const size_t baseSize = JSON_OBJECT_SIZE(6); // sensor_id, timestamp, count, raw_count, notes, raw_blocks
  const size_t notesArraySize = JSON_ARRAY_SIZE(bufferIndex);
  const size_t notesObjectsSize = bufferIndex * JSON_OBJECT_SIZE(5); // t,n,v,d,c
#if ENABLE_RAW_LOGGING
  const size_t rawArraySize = JSON_ARRAY_SIZE(rawCount);
  const size_t rawBlockEntrySize = JSON_OBJECT_SIZE(7); // t,max,min,avg,std,delta,threshold
  const size_t rawBlocksSize = rawCount * rawBlockEntrySize + rawArraySize;
#else
  const size_t rawBlocksSize = 0;
#endif
  const size_t capacity = baseSize + notesArraySize + notesObjectsSize + rawBlocksSize + 300;
  DynamicJsonDocument doc(capacity);
  
  // Metadata
  doc["sensor_id"] = sensorID;
  doc["timestamp"] = millis();
  doc["count"] = bufferIndex;
  doc["raw_count"] = rawCount;
  
  // Array de notas
  JsonArray notes = doc.createNestedArray("notes");
  
  for (int i = 0; i < bufferIndex; i++) {
    JsonObject note = notes.createNestedObject();
    note["t"] = midiBuffer[i].timestamp;
    note["n"] = midiBuffer[i].note;
    note["v"] = midiBuffer[i].velocity;
    note["d"] = midiBuffer[i].duration;
  }

#if ENABLE_RAW_LOGGING
  if (rawCount > 0) {
    JsonArray rawBlocks = doc.createNestedArray("raw_blocks");
    size_t index = rawBlockReadIndex;
    for (size_t i = 0; i < rawCount; i++) {
      const RawSampleBlock &block = rawBlockQueue[index];
      JsonObject rawObj = rawBlocks.createNestedObject();
      rawObj["t"] = block.timestamp;
      rawObj["max"] = block.maximum;
      rawObj["min"] = block.minimum;
      rawObj["avg"] = block.average;
      rawObj["std"] = block.stddev;
      rawObj["delta"] = block.delta;
      rawObj["threshold"] = block.threshold;

      index = (index + 1) % RAW_BLOCK_QUEUE_SIZE;
    }
  }
#endif
  
  // Serializar a String
  String payload;
  serializeJson(doc, payload);
  
  // Construir topic
  String topic = String(MQTT_BASE_TOPIC) + "/" + sensorID + "/midi";
  
  // Verificar tamaño del payload antes de enviar
  if (debugSerial) {
    Serial.print("→ Enviando ");
    Serial.print(bufferIndex);
    Serial.print(" notas");
#if ENABLE_RAW_LOGGING
    Serial.print(" y ");
    Serial.print(rawCount);
    Serial.print(" bloques crudos");
#endif
    Serial.print(", tamaño: ");
    Serial.print(payload.length());
    Serial.println(" bytes");
  }
  
  // Publicar en MQTT
  bool success = false;
  const uint8_t maxAttempts = MQTT_SEND_MAX_RETRIES + 1;
  uint8_t attempt = 0;

  while (attempt < maxAttempts) {
    if (attempt > 0 && debugSerial) {
      Serial.print("↻ Reintentando envío MQTT (intento ");
      Serial.print(attempt + 1);
      Serial.print(" de ");
      Serial.print(maxAttempts);
      Serial.println(")");
    }

    success = mqtt.publish(topic.c_str(), payload.c_str(), false); // QoS 0
    if (success) {
      break;
    }

    if (debugSerial) {
      Serial.print("✗ MQTT: Falló envío en intento ");
      Serial.print(attempt + 1);
      Serial.print(" - Estado MQTT: ");
      Serial.print(mqtt.state());
      Serial.print(" - Conectado: ");
      Serial.println(mqtt.connected() ? "SI" : "NO");
    }

    if (!mqtt.connected()) {
      reconnectMQTT();
      if (debugSerial && !mqtt.connected()) {
        Serial.println("⚠ MQTT: Reintento sin conexión activa");
      }
    }

    attempt++;
  }

  if (success) {
    if (debugSerial) {
      Serial.print("✓ MQTT: Envío exitoso (notas=");
      Serial.print(bufferIndex);
      Serial.print(", crudos=");
      Serial.print(rawCount);
      Serial.println(")");
    }
    
    // Limpiar buffers
    bufferIndex = 0;
#if ENABLE_RAW_LOGGING
    rawBlockCount = 0;
    rawBlockReadIndex = 0;
    rawBlockWriteIndex = 0;
#endif
    // wifiDisconnectedLogged = false;
  } else {
    if (debugSerial) {
      Serial.print("✗ MQTT: Último intento (");
      Serial.print(maxAttempts);
      Serial.println(") fallido, descartando buffer");
    }
    bufferIndex = 0;
#if ENABLE_RAW_LOGGING
    rawBlockCount = 0;
    rawBlockReadIndex = 0;
    rawBlockWriteIndex = 0;
#endif
    forceSendPending = false;
    lastBufferSend = millis();
  }

  return success;
}

// ============================================================================
// CHECK BUFFER TIMER - Verificar si es momento de enviar el buffer
// ============================================================================
void checkBufferTimer() {
  // Verificar si ha pasado el intervalo de envío
  if (millis() - lastBufferSend >= BUFFER_SEND_INTERVAL) {
    flushMQTTPayload();
    lastBufferSend = millis();
  }
}

#if ENABLE_RAW_LOGGING
void queueRawBlock(unsigned long timestamp,
                   unsigned long maximum,
                   unsigned long minimum,
                   unsigned long average,
                   float stddev,
                   unsigned long delta,
                   float thresholdValue) {
  if (rawBlockCount >= RAW_BLOCK_QUEUE_SIZE) {
    forceSendPending = true;
    flushMQTTPayload();
  }

  if (rawBlockCount >= RAW_BLOCK_QUEUE_SIZE) {
    rawBlockReadIndex = (rawBlockReadIndex + 1) % RAW_BLOCK_QUEUE_SIZE;
    rawBlockCount--;
    if (debugSerial) {
      Serial.println("⚠ Cola de bloques crudos llena; descartando el más antiguo");
    }
  }

  RawSampleBlock &block = rawBlockQueue[rawBlockWriteIndex];
  block.timestamp = timestamp;
  block.maximum = maximum;
  block.minimum = minimum;
  block.average = average;
  block.stddev = stddev;
  block.delta = delta;
  block.threshold = thresholdValue;

  rawBlockWriteIndex = (rawBlockWriteIndex + 1) % RAW_BLOCK_QUEUE_SIZE;
  rawBlockCount++;

  if (rawBlockCount >= RAW_BLOCK_QUEUE_SIZE) {
    forceSendPending = true;
  }
}
#endif

void flushMQTTPayload() {
#if ENABLE_RAW_LOGGING
  const bool hasRawBlocks = rawBlockCount > 0;
#else
  const bool hasRawBlocks = false;
#endif

  if (bufferIndex == 0 && !hasRawBlocks) {
    return;
  }

  const unsigned long now = millis();
  const bool intervalElapsed = (now - lastBufferSend) >= BUFFER_SEND_INTERVAL;

  if (!forceSendPending && !intervalElapsed) {
    return;
  }

  if (sendBufferToInflux()) {
    forceSendPending = false;
    lastBufferSend = now;
  }
}

