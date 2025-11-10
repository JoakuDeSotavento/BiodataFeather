/***************************************************
  NodeMCU v3 (ESP8266) + BME688 → MQTT cada 5 minutos
  Adaptado desde EnvironmentalData2InfluxDB (sin LTR329)
****************************************************/

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DFRobot_BME68x.h"
#include "secrets.h"

// ---------- Sensores ----------
DFRobot_BME68x_I2C bme(0x77);

// ---------- WiFi / MQTT ----------
WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
String sensorId;

// ---------- Timing ----------
unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 300000UL; // 5 minutos

// ---------- Prototipos ----------
void setup_wifi();
void reconnect_mqtt();
void read_sensors();
void send_mqtt(const String& topic, const String& payload);

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("NodeMCU v3 - BME688 MQTT 5 min");

  Wire.begin(D2, D1);

  uint8_t rslt = 1;
  while (rslt != 0) {
    rslt = bme.begin();
    if (rslt != 0) {
      Serial.println("BME68x no inicializado, reintentando...");
      delay(2000);
      yield();
    }
  }
  Serial.println("BME68x OK");

  bme.setGasHeater(320, 150); // Ajuste moderado para estabilidad en lecturas esporádicas

  sensorId = "";
#ifdef SENSOR_ID
  sensorId = SENSOR_ID;
#endif
  if (sensorId.length() == 0) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "env_mcu_%02X%02X%02X", mac[3], mac[4], mac[5]);
    sensorId = buffer;
  }
  Serial.print("Sensor ID: ");
  Serial.println(sensorId);
  
  setup_wifi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setKeepAlive(15);
  mqtt.setBufferSize(256);
}

// ---------- Loop ----------
void loop() {
  if (!mqtt.connected()) {
    reconnect_mqtt();
  }
  mqtt.loop();

  const unsigned long now = millis();
  if (lastRead == 0 || now - lastRead >= READ_INTERVAL) {
    lastRead = now;
    read_sensors();
  }
}

// ---------- Lectura de sensores ----------
void read_sensors() {
  bme.startConvert();
  delay(150);
  bme.update();

  float temperatura = bme.readTemperature() / 100.0f;
  float presion = bme.readPressure();
  float humedad = bme.readHumidity() / 1000.0f;
  float gas = bme.readGasResistance();
  float altitud = bme.readAltitude();

  Serial.println("--- Lectura BME688 ---");
  Serial.printf("Temp: %.2f °C\n", temperatura);
  Serial.printf("Pres: %.0f Pa\n", presion);
  Serial.printf("Hum: %.2f %%\n", humedad);
  Serial.printf("Gas: %.0f Ω\n", gas);
  Serial.printf("Alt: %.2f m\n", altitud);

  StaticJsonDocument<256> doc;
  doc["temperatura"] = temperatura;
  doc["presion"] = presion;
  doc["humedad"] = humedad;
  doc["gas"] = gas;
  doc["altitud"] = altitud;

  String mqttTopic = String(MQTT_BASE_TOPIC) + "/" + sensorId;
  char payload[256];
  size_t len = serializeJson(doc, payload, sizeof(payload));

  if (len == 0 || len >= sizeof(payload)) {
    Serial.println("Error: JSON MQTT demasiado grande");
    return;
  }

  send_mqtt(mqttTopic, payload);
}

// ---------- Envío a MQTT ----------
void send_mqtt(const String& topic, const String& payload) {
  if (mqtt.publish(topic.c_str(), payload.c_str(), false)) {
    Serial.print("Publicado MQTT -> ");
    Serial.println(topic);
    Serial.println(payload);
  } else {
    Serial.print("Error publicando en MQTT: ");
    Serial.println(topic);
  }
}

// ---------- WiFi ----------
void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Conectando a %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    yield();
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ---------- Reconexión MQTT ----------
void reconnect_mqtt() {
  while (!mqtt.connected()) {
    Serial.print("Conectando MQTT...");
    if (mqtt.connect(sensorId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("OK");
    } else {
      Serial.print("Falló, rc=");
      Serial.print(mqtt.state());
      Serial.println(" reintentando en 5s");
      delay(5000);
      yield();
    }
  }
}


