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
const char* influxToken = "RUWzp6k07mf9lsmmy1UVYCEPgmuU7FR1E6aJKMMM13VSqo24U2J2QBc0-zbjzcdPOLyDHJGXLHY-AQC-MhoLdw==";
