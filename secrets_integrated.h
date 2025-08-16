/*
 * Configuración WiFi y MQTT para BiodataFeather Integrated
 * 
 * IMPORTANTE: Cambia estos valores por tus credenciales reales
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

// ---------- Configuración opcional ----------
// Descomenta la siguiente línea para calibrar presión a nivel del mar
// #define CALIBRATE_PRESSURE

// ---------- Configuración de sensores ----------
#define SENSOR_READ_INTERVAL 5000    // 5 segundos
#define MQTT_SEND_INTERVAL 10000     // 10 segundos

// ---------- Configuración MIDI ----------
#define DEFAULT_MIDI_CHANNEL 1
#define DEFAULT_THRESHOLD 3
#define DEFAULT_SCALE 3  // 0=Cromática, 1=Menor, 2=Mayor, 3=Pentatónica, 4=India

// ---------- Configuración de LEDs ----------
#define LED_BRIGHTNESS_MAX 255
#define LED_BRIGHTNESS_DIM 50
#define LED_BRIGHTNESS_ACTIVITY 100

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
