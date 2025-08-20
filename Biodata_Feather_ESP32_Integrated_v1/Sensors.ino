// ---------- Configuración de sensores ----------
void setup_sensors() {
  Wire.begin();
  
  // Inicializar sensor de luz LTR329
  if (!ltr.begin()) {
    if(debugSerial) Serial.println("LTR329 no encontrado");
    while (1) delay(10);
  }
  if(debugSerial) Serial.println("LTR329 OK");
  
  ltr.setGain(LTR3XX_GAIN_2);
  ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
  ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
  
  // Inicializar sensor BME688
  uint8_t rslt = 1;
  while (rslt != 0) {
    rslt = bme.begin();
    if (rslt != 0) {
      if(debugSerial) Serial.println("BME68x begin failure");
      delay(2000);
    }
  }
  if(debugSerial) Serial.println("BME68x OK");
  
  // Configurar calentador de gas
  bme.setGasHeater(360, 100);
  
  // Calibrar presión a nivel del mar (opcional)
  #ifdef CALIBRATE_PRESSURE
    bme.startConvert();
    delay(1000);
    bme.update();
    seaLevel = bme.readSeaLevel(525.0);
    if (isnan(seaLevel) || seaLevel <= 0) {
      seaLevel = 101325.0; // 1 atm estándar
      if(debugSerial) Serial.println("Sea level inválido, usando 101325 Pa");
    } else {
      if(debugSerial) {
        Serial.print("Sea level OK: "); 
        Serial.println(seaLevel);
      }
    }
  #endif
}

// ---------- Lectura de sensores ----------
void read_sensors() {
  // Leer sensor BME688
  bme.startConvert();
  delay(100);
  bme.update();
  
  temperature = bme.readTemperature() / 100.0;  // Dividir por 100
  pressure = bme.readPressure();
  humidity = bme.readHumidity() / 1000.0;       // Dividir por 1000
  gas = bme.readGasResistance();
  altitude = bme.readAltitude();
  altitudeCal = bme.readCalibratedAltitude(seaLevel);
  
  // Proteger contra NaN o inf en altitud calibrada
  if (isnan(altitudeCal) || isinf(altitudeCal)) {
    altitudeCal = altitude; // fallback
    if(debugSerial) Serial.println("Altitud calibrada inválida, usando altitud normal");
  }
  
  if(debugSerial) {
    Serial.println("=== Valores Raw BME688 ===");
    Serial.printf("Temp Raw: %.2f°C\n", temperature);
    Serial.printf("Humedad Raw: %.2f%%\n", humidity);
    Serial.printf("Presión Raw: %.0f Pa\n", pressure);
    Serial.printf("Gas Raw: %.0f Ohm\n", gas);
    Serial.printf("Altitud Raw: %.2f m\n", altitude);
    Serial.println("==========================");
  }
  
  // Verificar que los valores sean razonables
  if (isnan(temperature) || temperature < -100 || temperature > 200) {
    if(debugSerial) Serial.println("Temperatura inválida, usando valor por defecto");
    temperature = 25.0; // Valor por defecto razonable
  }
  if (isnan(humidity) || humidity < -10 || humidity > 110) {
    if(debugSerial) Serial.println("Humedad inválida, usando valor por defecto");
    humidity = 50.0; // Valor por defecto razonable
  }
  if (isnan(pressure) || pressure < 80000 || pressure > 120000) {
    pressure = 101325.0;
  }
  if (isnan(gas) || gas < 0) {
    gas = 0.0;
  }
  
  // Leer sensor de luz LTR329
  uint16_t visible_plus_ir = 0;
  uint16_t infrared_ir = 0;
  bool valid = false;

  if (ltr.newDataAvailable()) {
    valid = ltr.readBothChannels(visible_plus_ir, infrared_ir);
  }

  if (valid) {
    visible = visible_plus_ir;
    infrared = infrared_ir;
    if(debugSerial) {
      Serial.printf("LTR329 - Visible+IR: %u, IR: %u\n", visible_plus_ir, infrared_ir);
    }
  } else {
    if(debugSerial) Serial.println("LTR329 - Error leyendo datos");
    visible = 0;
    infrared = 0;
  }
  
  if(debugSerial) {
    Serial.println("=== Datos de Sensores ===");
    Serial.printf("Temperatura: %.2f°C\n", temperature);
    Serial.printf("Presión: %.0f Pa\n", pressure);
    Serial.printf("Humedad: %.2f%%\n", humidity);
    Serial.printf("Gas: %.0f Ohm\n", gas);
    Serial.printf("Altitud: %.2f m\n", altitude);
    Serial.printf("Altitud Cal: %.2f m\n", altitudeCal);
    Serial.printf("Luz Visible: %d\n", visible);
    Serial.printf("Luz IR: %d\n", infrared);
    Serial.println("=========================");
  }
}

// ---------- Envío de datos MQTT ----------
void send_mqtt_data() {
  if (!mqtt.connected()) {
    if(debugSerial) Serial.println("MQTT no conectado, no se pueden enviar datos");
    return;
  }
  
  // Crear JSON con datos de sensores
  StaticJsonDocument<512> doc;
  
  doc["temperatura"] = temperature;
  doc["presion"] = pressure;
  doc["humedad"] = humidity;
  doc["gas"] = gas;
  doc["altitud"] = altitude;
  doc["altitudCalibrada"] = altitudeCal;
  doc["visible_ir"] = visible;
  doc["infrarrojo"] = infrared;
  doc["sensor"] = SENSOR_ID;
  doc["timestamp"] = millis();
  
  // Serializar JSON
  String payload;
  serializeJson(doc, payload);
  
  // Publicar en MQTT
  String topic = String(MQTT_BASE_TOPIC) + "/" + String(SENSOR_ID);
  if(debugSerial) {
    Serial.printf("Publicando en tópico: %s\n", topic.c_str());
  }
  bool published = mqtt.publish(topic.c_str(), payload.c_str());
  
  if(debugSerial) {
    if(published) {
      Serial.println("Datos enviados por MQTT:");
      Serial.println(payload);
    } else {
      Serial.println("Error enviando datos MQTT");
    }
  }
  
  // Indicar envío con LED
  setLED(2, 255); // LED verde
  delay(50);
  setLED(2, 0);
}
