#pragma once
// WiFi
#define WIFI_SSID "MolinoLab"
#define WIFI_PASSWORD "hacktheworld"
// MQTT
#define MQTT_BROKER "mqt.sinfoniabiotica.xyz"
#define MQTT_PORT 1883
#define MQTT_USER "biodata"
#define MQTT_PASSWORD "b10d4t4?"
#define MQTT_BASE_TOPIC "biodata"
#define SENSOR_ID "biodata1" // Cambia esto por cada nodo
const char* influxURL = "https://db.sinfoniabiotica.xyz:443/api/v2/write?org=MolinoLab&bucket=biodata&precision=s";
const char* influxToken = "0b2BkqoPhbVEg3yXNhYC09odJmLvSK8RlrjGndZiAS5wEeKqqNiG7ZVeP6U2MoRg86UsFgHTwBpq1_Ls4TsB9A==";
