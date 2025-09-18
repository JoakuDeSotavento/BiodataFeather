/*
 * EJEMPLO de configuración WiFi y MQTT para BiodataFeather Integrated
 * 
 * IMPORTANTE: 
 * 1. Copia este archivo a "secrets.h"
 * 2. Cambia los valores por tus credenciales reales
 * 3. NUNCA subas "secrets.h" al repositorio
 * 
 * SOLO información sensible (contraseñas, URLs, tokens)
 */

// ---------- WiFi ----------
#define WIFI_SSID "tu_wifi_ssid_aqui"
#define WIFI_PASSWORD "tu_wifi_password_aqui"

// ---------- MQTT ----------
#define MQTT_BROKER "tu_mqtt_broker.com"  // o IP como "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USERNAME "tu_usuario_mqtt"   // opcional
#define MQTT_PASSWORD "tu_password_mqtt"  // opcional

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
 * 
 * Para AWS IoT:
 * #define MQTT_BROKER "tu-endpoint.iot.region.amazonaws.com"
 * #define MQTT_PORT 8883
 * 
 * Para Google Cloud IoT:
 * #define MQTT_BROKER "mqtt.googleapis.com"
 * #define MQTT_PORT 8883
 */
