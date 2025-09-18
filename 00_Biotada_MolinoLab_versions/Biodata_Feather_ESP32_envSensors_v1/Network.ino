// ---------- Configuración WiFi ----------
void setup_wifi() {
  if(debugSerial) {
    Serial.print("Conectando a WiFi: ");
    Serial.println(WIFI_SSID);
  }
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    if(debugSerial) Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    if(debugSerial) {
      Serial.println();
      Serial.println("WiFi conectado");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("MAC: ");
      Serial.println(WiFi.macAddress());
    }
    
    // Indicar conexión WiFi con LED
    setLED(1, 255); // LED azul
    delay(100);
    setLED(1, 0);
  } else {
    if(debugSerial) {
      Serial.println();
      Serial.println("Error conectando WiFi");
    }
    
    // Indicar error con LED
    setLED(0, 255); // LED rojo
    delay(100);
    setLED(0, 0);
  }
}

// ---------- Reconexión MQTT ----------
void reconnect_mqtt() {
  while (!mqtt.connected()) {
      if(debugSerial) {
    Serial.print("Conectando MQTT a ");
    Serial.print(MQTT_BROKER);
    Serial.print(":");
    Serial.println(MQTT_PORT);
  }
    
    // Crear ID único para el cliente MQTT
    String clientId = "BiodataFeather-";
    clientId += String(random(0xffff), HEX);
    
    if (mqtt.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      if(debugSerial) {
        Serial.println("MQTT conectado exitosamente");
        Serial.print("Client ID: ");
        Serial.println(clientId);
      }
      
      // Suscribirse a temas de control (opcional)
      String controlTopic = String(MQTT_BASE_TOPIC) + "/control/" + String(SENSOR_ID);
      mqtt.subscribe(controlTopic.c_str());
      
      // Indicar conexión MQTT con LED
      setLED(3, 255); // LED blanco
      delay(100);
      setLED(3, 0);
      
    } else {
      if(debugSerial) {
        Serial.print("Error MQTT, rc=");
        Serial.print(mqtt.state());
        Serial.println(" reintentando en 5 segundos");
      }
      
      // Indicar error con LED
      setLED(0, 255);
      delay(100);
      setLED(0, 0);
      
      delay(5000);
    }
  }
}

// ---------- Callback MQTT (para recibir comandos) ----------
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  if(debugSerial) {
    Serial.print("Mensaje MQTT recibido [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  }
  
  // Procesar comandos recibidos
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // Ejemplo: procesar comandos JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (!error) {
    // Comando para cambiar threshold
    if (doc.containsKey("threshold")) {
      threshold = doc["threshold"];
      if(debugSerial) {
        Serial.print("Threshold cambiado a: ");
        Serial.println(threshold);
      }
    }
    
    // Comando para cambiar escala
    if (doc.containsKey("scale")) {
      int scaleIndex = doc["scale"];
      switch(scaleIndex) {
        case 0: scaleSelect = scaleChrom; break;
        case 1: scaleSelect = scaleMinor; break;
        case 2: scaleSelect = scaleMajor; break;
        case 3: scaleSelect = scalePenta; break;
        case 4: scaleSelect = scaleIndian; break;
      }
      if(debugSerial) {
        Serial.print("Escala cambiada a: ");
        Serial.println(scaleIndex);
      }
    }
    
    // Comando para cambiar canal MIDI
    if (doc.containsKey("midi_channel")) {
      channel = doc["midi_channel"];
      if(debugSerial) {
        Serial.print("Canal MIDI cambiado a: ");
        Serial.println(channel);
      }
    }
  }
  
  // Indicar recepción con LED
  setLED(4, 255); // LED amarillo
  delay(50);
  setLED(4, 0);
}
