// ============================================================================
// MQTT INFLUX BUFFER - Sistema de buffering de notas MIDI para envío a InfluxDB
// ============================================================================
// Este archivo contiene toda la lógica para almacenar notas MIDI en un buffer
// y enviarlas periódicamente vía MQTT a InfluxDB.
// No modifica ninguna funcionalidad MIDI existente.
// ============================================================================
// Nota: Los includes y secrets.h están en el archivo principal

// Configuración del buffer
#define MIDI_BUFFER_SIZE 100              // Tamaño máximo del buffer
#define BUFFER_SEND_INTERVAL 10000        // 10 segundos entre envíos

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

// Cliente MQTT
WiFiClient mqttWifiClient;
PubSubClient mqtt(mqttWifiClient);

// Tamaño máximo del buffer MQTT (por defecto es 256, aumentamos a 1024)
#define MQTT_MAX_PACKET_SIZE 1024

// Sensor ID (generado desde MAC)
String sensorID = "";

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
    // Formato: biodata_AABBCC (últimos 3 bytes del MAC en hex)
    sensorID = "biodata_";
    for (int i = 3; i < 6; i++) {
      if (mac[i] < 16) sensorID += "0";
      sensorID += String(mac[i], HEX);
    }
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
    sendBufferToInflux();
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
// SEND BUFFER TO INFLUX - Enviar buffer de notas vía MQTT
// ============================================================================
void sendBufferToInflux() {
  // Si buffer vacío, no enviar nada (OPCIÓN A)
  if (bufferIndex == 0) {
    return;
  }
  
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
      return;
    }
  }
  
  // Construir payload JSON
  // Calcular capacidad necesaria: base (100) + 80 bytes por nota
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(bufferIndex) + bufferIndex * JSON_OBJECT_SIZE(5) + 200;
  DynamicJsonDocument doc(capacity);
  
  // Metadata
  doc["sensor_id"] = sensorID;
  doc["timestamp"] = millis();
  doc["count"] = bufferIndex;
  
  // Array de notas
  JsonArray notes = doc.createNestedArray("notes");
  
  for (int i = 0; i < bufferIndex; i++) {
    JsonObject note = notes.createNestedObject();
    note["t"] = midiBuffer[i].timestamp;
    note["n"] = midiBuffer[i].note;
    note["v"] = midiBuffer[i].velocity;
    note["d"] = midiBuffer[i].duration;
    note["c"] = midiBuffer[i].midiChannel;
  }
  
  // Serializar a String
  String payload;
  serializeJson(doc, payload);
  
  // Construir topic
  String topic = String(MQTT_BASE_TOPIC) + "/" + sensorID + "/midi";
  
  // Verificar tamaño del payload antes de enviar
  if (debugSerial) {
    Serial.print("→ Enviando ");
    Serial.print(bufferIndex);
    Serial.print(" notas, tamaño: ");
    Serial.print(payload.length());
    Serial.println(" bytes");
  }
  
  // Publicar en MQTT
  bool success = mqtt.publish(topic.c_str(), payload.c_str(), false); // QoS 0
  
  if (success) {
    if (debugSerial) {
      Serial.print("✓ MQTT: Enviadas ");
      Serial.print(bufferIndex);
      Serial.print(" notas exitosamente");
      Serial.println();
    }
    
    // Limpiar buffer
    bufferIndex = 0;
  } else {
    if (debugSerial) {
      Serial.print("✗ MQTT: Falló envío - Estado MQTT: ");
      Serial.print(mqtt.state());
      Serial.print(" - Conectado: ");
      Serial.print(mqtt.connected() ? "SI" : "NO");
      Serial.println(" - Buffer retenido");
    }
    
    // Verificar si perdimos la conexión durante el envío
    if (!mqtt.connected()) {
      if (debugSerial) {
        Serial.println("⚠ Conexión perdida durante envío");
      }
    }
  }
}

// ============================================================================
// CHECK BUFFER TIMER - Verificar si es momento de enviar el buffer
// ============================================================================
void checkBufferTimer() {
  // Verificar si ha pasado el intervalo de envío
  if (millis() - lastBufferSend >= BUFFER_SEND_INTERVAL) {
    sendBufferToInflux();
    lastBufferSend = millis();
  }
}

