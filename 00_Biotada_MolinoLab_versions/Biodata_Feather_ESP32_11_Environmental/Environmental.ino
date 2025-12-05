// ============================================================================
// ENVIRONMENTAL.INO - Lectura y envío de datos ambientales (BME688 + LTR329)
// ============================================================================
// Integración de sensores ambientales para envío vía MQTT cada 5 minutos
// ============================================================================

#include "Adafruit_LTR329_LTR303.h"
#include "DFRobot_BME68x.h"
#include <Wire.h>
#include <ArduinoJson.h>
#include "secrets.h"

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
const unsigned long ENVIRONMENTAL_READ_INTERVAL = 300000;  // 5 minutos (300000 ms)

// Estado de inicialización
bool environmentalSensorsReady = false;
bool ltr329Ready = false;
bool bme688Ready = false;

// Sea level para calibración de presión (opcional)
#ifdef CALIBRATE_PRESSURE
float seaLevel = 101325.0;  // Presión estándar a nivel del mar (Pa)
#endif

// Prototipos de funciones
void setupEnvironmentalSensors();
void readEnvironmentalSensors();
void sendEnvironmentalData(float temp, float pres, float hum, float gas, float alt, uint16_t visible_ir, uint16_t infrared);

// ============================================================================
// SETUP ENVIRONMENTAL SENSORS - Inicialización de sensores ambientales
// ============================================================================
void setupEnvironmentalSensors() {
  if (debugSerial) {
    Serial.println("=== Inicializando Sensores Ambientales ===");
  }

  // Inicializar LTR329
  ltr329Ready = false;
  if (!ltr.begin()) {
    if (debugSerial) {
      Serial.println("✗ LTR329 no encontrado - Continuando sin LTR329");
    }
  } else {
    ltr329Ready = true;
    if (debugSerial) {
      Serial.println("✓ LTR329 OK");
    }
  }

  // Inicializar BME688 (continuar aunque LTR329 haya fallado)
  bme688Ready = false;
  uint8_t rslt = 1;
  uint8_t attempts = 0;
  while (rslt != 0 && attempts < 5) {
    rslt = bme.begin();
    if (rslt != 0) {
      if (debugSerial) {
        Serial.println("BME68x begin failure, reintentando...");
      }
      delay(2000);
      attempts++;
    }
  }

  if (rslt != 0) {
    if (debugSerial) {
      Serial.println("✗ BME68x no encontrado después de 5 intentos");
    }
    bme688Ready = false;
  } else {
    bme688Ready = true;
    if (debugSerial) {
      Serial.println("✓ BME68x OK");
    }
  }

  // Si ningún sensor está disponible, salir
  if (!ltr329Ready && !bme688Ready) {
    if (debugSerial) {
      Serial.println("✗ Ningún sensor ambiental disponible - Deshabilitando módulo ambiental");
    }
    environmentalSensorsReady = false;
    return;
  }

#ifdef CALIBRATE_PRESSURE
  // Calibrar presión a nivel del mar (opcional) - solo si BME688 está disponible
  if (bme688Ready) {
    bme.startConvert();
    delay(1000);
    bme.update();
    seaLevel = bme.readSeaLevel(525.0);
    if (isnan(seaLevel) || seaLevel <= 0) {
      seaLevel = 101325.0;  // 1 atm estándar
      if (debugSerial) {
        Serial.println("Sea level inválido, usando 101325 Pa");
      }
    } else {
      if (debugSerial) {
        Serial.print("Sea level OK: ");
        Serial.println(seaLevel);
      }
    }
  }
#endif

  // Configurar BME688 solo si está disponible
  if (bme688Ready) {
    bme.setGasHeater(360, 100);
  }

  // Configurar LTR329 DESPUÉS del BME688 (solo si está disponible)
  if (ltr329Ready) {
    ltr.setGain(LTR3XX_GAIN_2);
    ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
    ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
  }

  environmentalSensorsReady = true;
  lastEnvironmentalRead = currentMillis;

  if (debugSerial) {
    Serial.println("=== Sensores Ambientales Listos ===");
    Serial.print("LTR329: ");
    Serial.println(ltr329Ready ? "✓ Disponible" : "✗ No disponible");
    Serial.print("BME688: ");
    Serial.println(bme688Ready ? "✓ Disponible" : "✗ No disponible");
    Serial.print("Intervalo de lectura: ");
    Serial.print(ENVIRONMENTAL_READ_INTERVAL / 1000);
    Serial.println(" segundos");
  }
}

// ============================================================================
// READ ENVIRONMENTAL SENSORS - Leer datos de sensores ambientales
// ============================================================================
void readEnvironmentalSensors() {
  if (!environmentalSensorsReady) {
    return;
  }

  // Leer datos del BME688 (solo si está disponible)
  float temperatura = 0;
  float presion = 0;
  float humedad = 0;
  float gas = 0;
  float altitud = 0;

  if (bme688Ready) {
    // Iniciar conversión del BME688
    bme.startConvert();
    delay(100);  // Mínimo para estabilizar
    bme.update();

    // Leer datos del BME688
    temperatura = bme.readTemperature() / 100.0;  // Convertir de centésimas a grados
    presion = bme.readPressure();
    humedad = bme.readHumidity() / 1000.0;  // Convertir de milésimas a porcentaje
    gas = bme.readGasResistance();
    altitud = bme.readAltitude();
  }

  // Leer datos del LTR329 (solo si está disponible)
  uint16_t visible_plus_ir = 0;
  uint16_t infrared = 0;
  bool valid = false;

  if (ltr329Ready && ltr.newDataAvailable()) {
    valid = ltr.readBothChannels(visible_plus_ir, infrared);
  }

  // Debug
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

  // Enviar datos vía MQTT
  if (mqtt.connected()) {
    sendEnvironmentalData(temperatura, presion, humedad, gas, altitud, visible_plus_ir, infrared);
  } else {
    if (debugSerial) {
      Serial.println("⚠ MQTT no conectado - Datos ambientales no enviados");
    }
  }
}

// ============================================================================
// SEND ENVIRONMENTAL DATA - Enviar datos ambientales vía MQTT
// ============================================================================
void sendEnvironmentalData(float temp, float pres, float hum, float gas, float alt, uint16_t visible_ir, uint16_t infrared) {
  // Crear JSON con datos ambientales
  StaticJsonDocument<256> doc;
  doc["temperatura"] = temp;
  doc["presion"] = pres;
  doc["humedad"] = hum;
  doc["gas"] = gas;
  doc["altitud"] = alt;
  doc["visible_ir"] = visible_ir;
  doc["infrarrojo"] = infrared;

  // Construir topic MQTT
  String mqttTopic = String(MQTT_ENV_TOPIC) + "/" + sensorID;

  // Serializar JSON
  char mqttPayload[256];
  serializeJson(doc, mqttPayload);

  // Enviar vía MQTT
  bool success = mqtt.publish(mqttTopic.c_str(), mqttPayload, false);  // QoS 0

  if (debugSerial) {
    if (success) {
      Serial.print("✓ Datos ambientales enviados a: ");
      Serial.println(mqttTopic);
    } else {
      Serial.print("✗ Error al enviar datos ambientales - Estado MQTT: ");
      Serial.println(mqtt.state());
    }
  }
}

// ============================================================================
// CHECK ENVIRONMENTAL TIMER - Verificar si es momento de leer sensores
// ============================================================================
void checkEnvironmentalTimer() {
  if (!environmentalSensorsReady) {
    return;
  }

  // Verificar si ha pasado el intervalo de lectura (5 minutos)
  if (currentMillis - lastEnvironmentalRead >= ENVIRONMENTAL_READ_INTERVAL) {
    lastEnvironmentalRead = currentMillis;
    readEnvironmentalSensors();
  }
}
