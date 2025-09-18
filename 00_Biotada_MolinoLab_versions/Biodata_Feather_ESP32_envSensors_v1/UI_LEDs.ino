// ---------- Funciones de LEDs ----------
void setLED(byte led, byte brightness) {
  if(led < 5) {
    if(brightness > 0) {
      digitalWrite(leds[led], HIGH);
    } else {
      digitalWrite(leds[led], LOW);
    }
  }
}

void updateLEDs() {
  // Actualizar LEDs basado en estado del sistema
  
  // LED 0 (Rojo) - Estado de error/conexión
  if(WiFi.status() != WL_CONNECTED) {
    if(blinkToggle) {
      setLED(0, 255);
    } else {
      setLED(0, 0);
    }
  } else {
    setLED(0, 0);
  }
  
  // LED 1 (Azul) - Estado WiFi
  if(WiFi.status() == WL_CONNECTED) {
    setLED(1, 50); // Luz tenue cuando conectado
  } else {
    setLED(1, 0);
  }
  
  // LED 2 (Verde) - Estado MQTT
  if(mqtt.connected()) {
    setLED(2, 50); // Luz tenue cuando conectado
  } else {
    setLED(2, 0);
  }
  
  // LED 3 (Blanco) - Actividad de sensores
  if(currentMillis - lastSensorRead < 1000) {
    setLED(3, 30); // Luz tenue cuando se leen sensores
  } else {
    setLED(3, 0);
  }
  
  // LED 4 (Amarillo) - Actividad MIDI
  bool midiActivity = false;
  for(int i = 0; i < polyphony; i++) {
    if(noteArray[i].velocity > 0) {
      midiActivity = true;
      break;
    }
  }
  
  if(midiActivity) {
    setLED(4, 100);
  } else {
    setLED(4, 0);
  }
  
  // Actualizar blink toggle cada 500ms
  if(currentMillis - blinkTime > 500) {
    blinkToggle = !blinkToggle;
    blinkTime = currentMillis;
  }
}

// ---------- Función para mostrar estado del sistema ----------
void showSystemStatus() {
  if(debugSerial) {
    Serial.println("\n=== ESTADO DEL SISTEMA ===");
    Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Conectado" : "Desconectado");
    Serial.printf("MQTT: %s\n", mqtt.connected() ? "Conectado" : "Desconectado");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("Canal MIDI: %d\n", channel);
    Serial.printf("Threshold: %d\n", threshold);
    Serial.printf("Temperatura: %.2f°C\n", temperature);
    Serial.printf("Humedad: %.2f%%\n", humidity);
    Serial.printf("Presión: %.0f Pa\n", pressure);
    Serial.printf("Gas: %.0f Ohm\n", gas);
    Serial.printf("Luz Visible: %d\n", visible);
    Serial.printf("Luz IR: %d\n", infrared);
    Serial.println("==========================\n");
  }
  
  // Mostrar estado con LEDs
  for(byte i = 0; i < 5; i++) {
    setLED(i, 255);
    delay(100);
    setLED(i, 0);
  }
}

// ---------- Función para mostrar configuración ----------
void showConfiguration() {
  if(debugSerial) {
    Serial.println("\n=== CONFIGURACIÓN ===");
    Serial.printf("Escala: ");
    if(scaleSelect == scaleChrom) Serial.println("Cromática");
    else if(scaleSelect == scaleMinor) Serial.println("Menor");
    else if(scaleSelect == scaleMajor) Serial.println("Mayor");
    else if(scaleSelect == scalePenta) Serial.println("Pentatónica");
    else if(scaleSelect == scaleIndian) Serial.println("India");
    Serial.printf("Canal MIDI: %d\n", channel);
    Serial.printf("Threshold: %d\n", threshold);
    Serial.printf("Nota Min: %d\n", noteMin);
    Serial.printf("Nota Max: %d\n", noteMax);
    Serial.printf("Control Number: %d\n", controlNumber);
    Serial.println("=====================\n");
  }
}

// ---------- Función para mostrar información de batería ----------
void showBatteryLevel() {
  // Leer voltaje de batería (pin A13)
  int batteryRaw = analogRead(A13);
  float batteryVoltage = (batteryRaw / 4095.0) * 3.3 * 2; // Factor 2 porque A13 lee la mitad del voltaje
  
  int batteryPercent = 0;
  if(batteryVoltage >= 4.0) batteryPercent = 100;
  else if(batteryVoltage >= 3.8) batteryPercent = 80;
  else if(batteryVoltage >= 3.6) batteryPercent = 60;
  else if(batteryVoltage >= 3.4) batteryPercent = 40;
  else if(batteryVoltage >= 3.2) batteryPercent = 20;
  else batteryPercent = 0;
  
  if(debugSerial) {
    Serial.printf("Batería: %.2fV (%d%%)\n", batteryVoltage, batteryPercent);
  }
  
  // Mostrar nivel de batería con LEDs
  setLED(4, 255); // LED blanco siempre encendido
  if(batteryPercent >= 80) {
    setLED(1, 255); // Azul
  } else if(batteryPercent >= 60) {
    setLED(2, 255); // Verde
  } else if(batteryPercent >= 40) {
    setLED(3, 255); // Amarillo
  } else if(batteryPercent >= 20) {
    setLED(0, 255); // Rojo
  }
  
  delay(2000);
  
  // Apagar todos los LEDs
  for(byte i = 0; i < 5; i++) {
    setLED(i, 0);
  }
}
