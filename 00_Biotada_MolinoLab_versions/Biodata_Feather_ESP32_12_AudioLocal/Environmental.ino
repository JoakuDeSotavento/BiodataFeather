// ============================================================================
// ENVIRONMENTAL.INO - Lectura y envío de datos ambientales (BME688 + LTR329)
// ============================================================================
// Si no hay sensores conectados, se detecta una sola vez y se omite todo el
// trabajo periódico (sin reintentos I2C ni delays que ralenticen MIDI/biodata).
// ============================================================================

#include "Adafruit_LTR329_LTR303.h"
#include "DFRobot_BME68x.h"
#include <WiFi.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "secrets.h"

#ifndef MQTT_ENV_TOPIC
#define MQTT_ENV_TOPIC "environmental"
#endif

// Variables externas
extern PubSubClient mqtt;
extern String sensorID;
extern unsigned long currentMillis;
extern byte debugSerial;

// Sensores ambientales
Adafruit_LTR329 ltr;
DFRobot_BME68x_I2C bme(0x77);  // I2C address 0x77

// Timing para lectura ambiental
unsigned long lastEnvironmentalRead = 0;
const unsigned long ENVIRONMENTAL_READ_INTERVAL = 300000;  // 5 minutos

// Estado: probe único al arranque / al activar WiFi
bool environmentalProbed = false;      // ya se intentó detectar hardware
bool environmentalSensorsReady = false;  // al menos un sensor usable
bool ltr329Ready = false;
bool bme688Ready = false;

#ifdef CALIBRATE_PRESSURE
float seaLevel = 101325.0;
#endif

void setupEnvironmentalSensors();
void readEnvironmentalSensors();
void sendEnvironmentalData(float temp, float pres, float hum, float gas, float alt, uint16_t visible_ir, uint16_t infrared);
void checkEnvironmentalTimer();

// ============================================================================
// SETUP ENVIRONMENTAL SENSORS - Una sola sonda; si falla, no se vuelve a llamar
// ============================================================================
void setupEnvironmentalSensors() {
  // Si ya se sondeó y no hay hardware: no reintentar (evita delays/I2C en loop)
  if (environmentalProbed && !environmentalSensorsReady) {
    return;
  }
  // Si ya están listos, no reinicializar
  if (environmentalProbed && environmentalSensorsReady) {
    return;
  }

  environmentalProbed = true;
  ltr329Ready = false;
  bme688Ready = false;
  environmentalSensorsReady = false;

  if (debugSerial) {
    Serial.println("=== Inicializando Sensores Ambientales ===");
  }

  // LTR329 — un solo intento (begin falla rápido si no hay dispositivo)
  if (ltr.begin()) {
    ltr329Ready = true;
    if (debugSerial) Serial.println("✓ LTR329 OK");
  } else if (debugSerial) {
    Serial.println("✗ LTR329 no encontrado");
  }

  // BME688 — un solo intento, sin bucle de reintentos con delay
  uint8_t rslt = bme.begin();
  if (rslt == 0) {
    bme688Ready = true;
    if (debugSerial) Serial.println("✓ BME68x OK");
  } else if (debugSerial) {
    Serial.println("✗ BME68x no encontrado");
  }

  if (!ltr329Ready && !bme688Ready) {
    if (debugSerial) {
      Serial.println("✗ Sin sensores ambientales — se omite toda la lógica (sin reintentos)");
    }
    environmentalSensorsReady = false;
    return;
  }

#ifdef CALIBRATE_PRESSURE
  if (bme688Ready) {
    bme.startConvert();
    delay(1000);
    bme.update();
    seaLevel = bme.readSeaLevel(525.0);
    if (isnan(seaLevel) || seaLevel <= 0) {
      seaLevel = 101325.0;
      if (debugSerial) Serial.println("Sea level inválido, usando 101325 Pa");
    } else if (debugSerial) {
      Serial.print("Sea level OK: ");
      Serial.println(seaLevel);
    }
  }
#endif

  if (bme688Ready) {
    bme.setGasHeater(360, 100);
  }

  if (ltr329Ready) {
    ltr.setGain(LTR3XX_GAIN_2);
    ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
    ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
  }

  environmentalSensorsReady = true;
  lastEnvironmentalRead = millis();

  if (debugSerial) {
    Serial.println("=== Sensores Ambientales Listos ===");
    Serial.print("LTR329: ");
    Serial.println(ltr329Ready ? "✓" : "✗");
    Serial.print("BME688: ");
    Serial.println(bme688Ready ? "✓" : "✗");
  }
}

// ============================================================================
// READ ENVIRONMENTAL SENSORS
// ============================================================================
void readEnvironmentalSensors() {
  if (!environmentalSensorsReady) {
    return;
  }

  float temperatura = 0;
  float presion = 0;
  float humedad = 0;
  float gas = 0;
  float altitud = 0;

  if (bme688Ready) {
    bme.startConvert();
    delay(100);  // estabilización BME (solo cada 5 min si hay sensor)
    bme.update();
    temperatura = bme.readTemperature() / 100.0;
    presion = bme.readPressure();
    humedad = bme.readHumidity() / 1000.0;
    gas = bme.readGasResistance();
    altitud = bme.readAltitude();
  }

  uint16_t visible_plus_ir = 0;
  uint16_t infrared = 0;

  if (ltr329Ready && ltr.newDataAvailable()) {
    ltr.readBothChannels(visible_plus_ir, infrared);
  }

  if (debugSerial) {
    Serial.println("--- Lectura Ambiental ---");
    Serial.printf("Temp: %.2f °C\n", temperatura);
    Serial.printf("Pres: %.0f Pa\n", presion);
    Serial.printf("Hum: %.2f %%\n", humedad);
    Serial.printf("Gas: %.0f Ω\n", gas);
    Serial.printf("Alt: %.2f m\n", altitud);
    Serial.printf("Visible+IR: %u\n", visible_plus_ir);
    Serial.printf("Infrared: %u\n", infrared);
  }

  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
    sendEnvironmentalData(temperatura, presion, humedad, gas, altitud, visible_plus_ir, infrared);
  } else if (debugSerial) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠ WiFi sin conexión - ambientales no enviados");
    } else {
      Serial.println("⚠ MQTT no conectado - ambientales no enviados");
    }
  }
}

// ============================================================================
// SEND ENVIRONMENTAL DATA
// ============================================================================
void sendEnvironmentalData(float temp, float pres, float hum, float gas, float alt, uint16_t visible_ir, uint16_t infrared) {
  StaticJsonDocument<256> doc;
  doc["temperatura"] = temp;
  doc["presion"] = pres;
  doc["humedad"] = hum;
  doc["gas"] = gas;
  doc["altitud"] = alt;
  doc["visible_ir"] = visible_ir;
  doc["infrarrojo"] = infrared;

  String mqttTopic = String(MQTT_ENV_TOPIC) + "/" + sensorID;

  char mqttPayload[256];
  serializeJson(doc, mqttPayload);

  bool success = mqtt.publish(mqttTopic.c_str(), mqttPayload, false);

  if (debugSerial) {
    if (success) {
      Serial.print("✓ Datos ambientales enviados a: ");
      Serial.println(mqttTopic);
    } else {
      Serial.print("✗ Error ambientales MQTT: ");
      Serial.println(mqtt.state());
    }
  }
}

// ============================================================================
// CHECK ENVIRONMENTAL TIMER — no-op si no hay sensores
// ============================================================================
void checkEnvironmentalTimer() {
  if (!environmentalSensorsReady) {
    return;  // sin hardware: cero coste en el loop
  }

  if (currentMillis - lastEnvironmentalRead >= ENVIRONMENTAL_READ_INTERVAL) {
    lastEnvironmentalRead = currentMillis;
    readEnvironmentalSensors();
  }
}
