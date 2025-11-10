/***************************************************
  AZ-Envy v4.1 (ESP8266) → MQTT (cada 5 minutos)
  Sensores integrados: SHT30 (I2C) + MQ-2 (analógico)
  Reutiliza credenciales definidas en secrets.h
****************************************************/

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SHT31.h>
#include "secrets.h"

// ---------- Sensores ----------
Adafruit_SHT31 sht30 = Adafruit_SHT31();
const int MQ2_PIN = A0;

// ---------- WiFi / MQTT ----------
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
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
  Serial.begin(115200);
  delay(500);
  Serial.println("AZ-Envy v4.1 - MQTT cada 5 minutos");

  Wire.begin();

  if (!sht30.begin(0x44)) {
    Serial.println("SHT30 no encontrado en 0x44");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("SHT30 OK");
  sht30.heater(false);

  pinMode(MQ2_PIN, INPUT);

  setup_wifi();

  sensorId = "";
#ifdef SENSOR_ID
  sensorId = SENSOR_ID;
#endif
  if (sensorId.length() == 0) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buffer[18];
    snprintf(buffer, sizeof(buffer), "env_%02X%02X%02X", mac[3], mac[4], mac[5]);
    sensorId = buffer;
  }
  Serial.print("Sensor ID: ");
  Serial.println(sensorId);

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
  if (now - lastRead >= READ_INTERVAL || lastRead == 0) {
    lastRead = now;
    read_sensors();
  }
}

// ---------- Lectura de sensores ----------
void read_sensors() {
  float temperatura = sht30.readTemperature();
  float humedad = sht30.readHumidity();

  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Lectura SHT30 inválida");
    return;
  }

  const uint8_t samples = 8;
  uint32_t mq2Acumulado = 0;
  for (uint8_t i = 0; i < samples; i++) {
    mq2Acumulado += analogRead(MQ2_PIN);
    delay(5);
  }
  const float mq2Promedio = mq2Acumulado / static_cast<float>(samples);
  const float mq2Ratio = mq2Promedio / 1023.0f;

  Serial.println("--- Lectura AZ-Envy ---");
  Serial.printf("Temp: %.2f °C\n", temperatura);
  Serial.printf("Hum: %.2f %%\n", humedad);
  Serial.printf("MQ-2 raw: %.0f\n", mq2Promedio);
  Serial.printf("MQ-2 ratio: %.3f\n", mq2Ratio);

  StaticJsonDocument<256> doc;
  doc["temperatura_c"] = temperatura;
  doc["humedad_rel"] = humedad;
  doc["mq2_raw"] = mq2Promedio;
  doc["mq2_ratio"] = mq2Ratio;

  String mqttTopic = String(MQTT_BASE_TOPIC) + "/" + sensorId;
  char mqttPayload[256];
  size_t len = serializeJson(doc, mqttPayload, sizeof(mqttPayload));

  if (len == 0 || len >= sizeof(mqttPayload)) {
    Serial.println("Error serializando JSON MQTT");
    return;
  }

  send_mqtt(mqttTopic, mqttPayload);
}

// ---------- Envío a MQTT ----------
void send_mqtt(const String& topic, const String& payload) {
  if (mqtt.publish(topic.c_str(), payload.c_str(), false)) {
    Serial.print("MQTT enviado a ");
    Serial.println(topic);
    Serial.println(payload);
  } else {
    Serial.print("Fallo al publicar en ");
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
    }
  }
}


