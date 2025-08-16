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
  
  temperature = bme.readTemperature();
  pressure = bme.readPressure();
  humidity = bme.readHumidity();
  gas = bme.readGasResistance();
  altitude = bme.readAltitude(); // Sin parámetros
  altitudeCal = altitude; // Usar el mismo valor por ahora
  
  // Verificar que los valores sean razonables
  if (isnan(temperature) || temperature < -50 || temperature > 100) {
    temperature = 0.0;
  }
  if (isnan(humidity) || humidity < 0 || humidity > 100) {
    humidity = 0.0;
  }
  if (isnan(pressure) || pressure < 80000 || pressure > 120000) {
    pressure = 101325.0;
  }
  if (isnan(gas) || gas < 0) {
    gas = 0.0;
  }
  
  // Leer sensor de luz LTR329
  uint16_t visible_ir, infrared_ir;
  if (ltr.newDataAvailable()) {
    bool valid = ltr.readBothChannels(visible_ir, infrared_ir);
    if (valid) {
      visible = visible_ir;
      infrared = infrared_ir;
    }
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
  
  doc["temperatura"] = round(temperature * 100) / 100.0;
  doc["presion"] = (int)pressure;
  doc["humedad"] = round(humidity * 100) / 100.0;
  doc["gas"] = (int)gas;
  doc["altitud"] = round(altitude * 100) / 100.0;
  doc["altitudCalibrada"] = round(altitudeCal * 100) / 100.0;
  doc["visible_ir"] = visible;
  doc["infrarrojo"] = infrared;
  doc["sensor"] = "biodata_feather";
  doc["timestamp"] = millis();
  
  // Serializar JSON
  String payload;
  serializeJson(doc, payload);
  
  // Publicar en MQTT
  String topic = String(MQTT_BASE_TOPIC) + "/sensors/" + String(SENSOR_ID);
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
