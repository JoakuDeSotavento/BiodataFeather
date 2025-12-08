/***************************************************
  ESP32 + BME688 + LTR329 → InfluxDB v2 (HTTPS) + MQTT
  Optimizado para envíos frecuentes (< 1 s)
  Conserva nombres y variables originales
****************************************************/

#include "Adafruit_LTR329_LTR303.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "DFRobot_BME68x.h"
#include <Wire.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

// ---------- Sensores ----------
Adafruit_LTR329 ltr;
DFRobot_BME68x_I2C bme(0x77); // I2C address

// ---------- WiFi / TLS ----------
WiFiClientSecure tlsClient;
WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
String sensorId;

// ---------- Timing ----------
unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 300000 ; // 5 mins

// ---------- Sea level ----------
float seaLevel;

// ---------- Prototipos ----------
void setup_wifi();
void reconnect_mqtt();
void read_sensors();
void send_influx(const String& line);
void send_mqtt(const String& topic, const String& payload);

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Optimizado: BME688 + LTR329");

  // Inicializar sensores
  if (!ltr.begin()) {
    Serial.println("LTR329 no encontrado");
    while (1) delay(10);
  }
  Serial.println("LTR329 OK");

  uint8_t rslt = 1;
  while (rslt != 0) {
    rslt = bme.begin();
    if (rslt != 0) {
      Serial.println("BME68x begin failure");
      delay(2000);
    }
  }
  Serial.println("BME68x OK");

#ifdef CALIBRATE_PRESSURE
  bme.startConvert();
  delay(1000);
  bme.update();
  seaLevel = bme.readSeaLevel(525.0);
  if (isnan(seaLevel) || seaLevel <= 0) {
    seaLevel = 101325.0; // 1 atm estándar
    Serial.println("Sea level inválido, usando 101325 Pa");
  } else {
    Serial.print("Sea level OK: "); Serial.println(seaLevel);
  }
#endif

  bme.setGasHeater(360, 100);

  ltr.setGain(LTR3XX_GAIN_2);
  ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
  ltr.setMeasurementRate(LTR3XX_MEASRATE_200);

  // WiFi
  setup_wifi();

  sensorId = "";
#ifdef SENSOR_ID
  sensorId = SENSOR_ID;
#endif
  if (sensorId.length() == 0) {
    byte mac[6];
    WiFi.macAddress(mac);
    int uniq = 0;
    for (int i = 0; i < 6; i++) {
      uniq += mac[i];
    }
    sensorId = "env_";
    sensorId += String(uniq);
  }

  Serial.println(sensorId);

  // TLS
  tlsClient.setInsecure(); // Solo para pruebas

  // MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setKeepAlive(15); // Mantener vivo
}

// ---------- Loop ----------
void loop() {
  if (!mqtt.connected()) {
    reconnect_mqtt();
  }
  mqtt.loop(); // Mantiene vivo el cliente MQTT

  if (millis() - lastRead >= READ_INTERVAL) {
    lastRead = millis();
    read_sensors();
  }
}

// ---------- Lectura de sensores ----------
void read_sensors() {
  bme.startConvert();
  delay(100); // Mínimo para estabilizar
  bme.update();

  float temperatura = bme.readTemperature() / 100.0;
  float presion = bme.readPressure();
  float humedad = bme.readHumidity() / 1000.0;
  float gas = bme.readGasResistance();
  float altitud = bme.readAltitude();
  //float altitudCalibrada = bme.readCalibratedAltitude(seaLevel);

  uint16_t visible_plus_ir = 0;
  uint16_t infrared = 0;
  bool valid = false;

  if (ltr.newDataAvailable()) {
    valid = ltr.readBothChannels(visible_plus_ir, infrared);
  }

  // Debug
  Serial.println("--- Lectura ---");
  Serial.printf("Temp: %.2f °C\n", temperatura);
  Serial.printf("Pres: %.0f Pa\n", presion);
  Serial.printf("Hum: %.2f %%\n", humedad);
  Serial.printf("Gas: %.0f Ω\n", gas);
  Serial.printf("Alt: %.2f m\n", altitud);
  //Serial.printf("AltCal: %.2f m\n", altitudCalibrada);
  Serial.printf("Visible+IR: %u\n", visible_plus_ir);
  Serial.printf("Infrared: %u\n", infrared);

  // MQTT JSON
  StaticJsonDocument<256> doc;
  doc["temperatura"] = temperatura;
  doc["presion"] = presion;
  doc["humedad"] = humedad;
  doc["gas"] = gas;
  doc["altitud"] = altitud;
  //doc["altitudCalibrada"] = altitudCalibrada;
  doc["visible_ir"] = visible_plus_ir;
  doc["infrarrojo"] = infrared;

  String mqttTopic = String(MQTT_BASE_TOPIC) + "/" + sensorId;
  char mqttPayload[256];
  serializeJson(doc, mqttPayload);

  send_mqtt(mqttTopic, mqttPayload);
}
// ---------- Envío a MQTT ----------
void send_mqtt(const String& topic, const String& payload) {
  mqtt.publish(topic.c_str(), payload.c_str(), false); // QoS 0
}

// ---------- WiFi ----------
void setup_wifi() {
  delay(10);
  Serial.printf("Conectando a %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

// ---------- Reconexión MQTT ----------
void reconnect_mqtt() {
  while (!mqtt.connected()) {
    Serial.print("Conectando MQTT...");
    if (mqtt.connect(sensorId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("OK");
    } else {
      Serial.print("Falló, rc="); Serial.print(mqtt.state());
      Serial.println(" reintentando en 5s");
      delay(5000);
    }
  }
}