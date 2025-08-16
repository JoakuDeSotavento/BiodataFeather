/*
 * Configuración WiFi y MQTT para BiodataFeather Integrated
 * 
 * IMPORTANTE: Cambia estos valores por tus credenciales reales
 * SOLO información sensible (contraseñas, URLs, tokens)
 */

// ---------- WiFi ----------
#define WIFI_SSID "MolinoLab"
#define WIFI_PASSWORD "hacktheworld"

// ---------- MQTT ----------
#define MQTT_BROKER "mqt.sinfoniabiotica.xyz"  // o IP como "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USERNAME "biodata"   // opcional
#define MQTT_PASSWORD "b10d4t4?"  // opcional
#define MQTT_BASE_TOPIC "biodata"
#define SENSOR_ID "biodata1" // Cambia esto por cada nodo

/*
 * Ejemplos de configuración:
 * 
 * Para Node-RED local:
 * #define MQTT_BROKER "192.168.1.100"
 * #define MQTT_PORT 1883
 * 
 * Para HiveMQ (gratuito):
 * #define MQTT_BROKER "broker.hivemq.com"
 * #define MQTT_PORT 1883
 * 
 * Para Mosquitto local:
 * #define MQTT_BROKER "localhost"
 * #define MQTT_PORT 1883
 */
