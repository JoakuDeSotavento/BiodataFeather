//   ESP32 S3 - BiodataFeather Integrated Version
//Electricity for Progress
// electricityforprogress.com
// **-----------------------------------------------------------------------------
//ESP32-S3 UDP/RTP AppleMIDI over wifi + Environmental Sensors + MQTT
//Biodata Sonification pulse input data via interrupt
//Environmental sensors via I2C (BME688 + LTR329)
//MQTT publishing to Node-RED for real-time data
//Knob and button interface
//LEDs using PWM fading
//ESP32 Feather A13 pin reads half of battery voltage (4.2 (3.7) -3.2V)
// **-----------------------------------------------------------------------------

#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// Sensores medioambientales
#include "Adafruit_LTR329_LTR303.h"
#include "DFRobot_BME68x.h"

// MIDI
#include <Adafruit_TinyUSB.h>

// Configuración WiFi y MQTT
#include "secrets.h"

// ---------- Configuración del sistema ----------
#define MQTT_BASE_TOPIC "biodata"
#define SENSOR_ID "biodata1" // Cambia esto por cada nodo

// ---------- InfluxDB (opcional, para envío directo) ----------
const char* influxURL = "https://db.sinfoniabiotica.xyz:443/api/v2/write?org=MolinoLab&bucket=biodata&precision=s";
const char* influxToken = "0b2BkqoPhbVEg3yXNhYC09odJmLvSK8RlrjGndZiAS5wEeKqqNiG7ZVeP6U2MoRg86UsFgHTwBpq1_Ls4TsB9A==";
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

// ---------- Configuración opcional ----------
// Descomenta la siguiente línea para calibrar presión a nivel del mar
// #define CALIBRATE_PRESSURE

// ---------- Sensores ----------
Adafruit_LTR329 ltr;
DFRobot_BME68x_I2C bme(0x77); // I2C address

// ---------- MQTT ----------
WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);

// ---------- Timing ----------
unsigned long lastSensorRead = 0;
unsigned long lastMQTTSend = 0;

// ---------- Variables de sensores ----------
float temperature, pressure, humidity, gas, altitude, altitudeCal;
int visible, infrared;
float seaLevel = 101325.0; // Presión estándar

// Configuración MIDI
byte channel = 1;
IPAddress local_IP(192, 168, 1, 36);
bool staticIP = false;

//MIDI Note and Controls
const byte polyphony = 5;

//******************************
//set scaled values, sorted array, first element scale length
int scalePenta[] = { 5, 0, 3, 5, 7, 9 };
int scaleMajor[] = { 7, 0, 2, 4, 5, 7, 9, 11 };
int scaleIndian[] = { 7, 0, 1, 1, 4, 5, 8, 10 };
int scaleMinor[] = { 7, 0, 2, 3, 5, 7, 8, 10 };
int scaleChrom[] = { 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
int *scaleSelect = scaleChrom;
byte defScale = 3;

int root = 0;

//Debug and MIDI output Settings
byte debugSerial = 1;
byte rawSerial = 1;
byte serialMIDI = 1;
byte wifiMIDI = 0;
byte bleMIDI = 1;
byte usbmidi = 1;
byte midiMode = 0;
byte wifiActive = 0;
byte bleActive = 0;
byte midiControl = channel;

int noteMin = 36;
int noteMax = 96;
byte controlNumber = 80;
unsigned long rawSerialTime = 0;
int rawSerialDelay = 10;

// EEPROM
#define EEPROM_SIZE 5

// I/O Pin declarations
int buttonPin = 13;
int potPin = A2;
const byte interruptPin = 44;

// LEDs
byte leds[5] = { 18, 17, 8, 36, 35 };
byte ledBrightness[5] = { 50, 110, 120, 120, 60 };
byte maxBrightness = 60;
bool blinkToggle = 0;
unsigned long blinkTime = 0;

// Variables de análisis de muestras
const byte samplesize = 32;
const byte analysize = 31;
unsigned long samples[samplesize];
byte sampleIndex = 0;
unsigned long microseconds = 0;
unsigned long currentMillis = 0;
int threshold = 3;

// Estructuras MIDI
struct Note {
  byte type;
  byte value;
  byte velocity;
  unsigned long duration;
  byte channel;
};

struct Control {
  byte type;
  byte value;
  byte velocity;
  unsigned long period;
  unsigned long duration;
};

Note noteArray[polyphony];
Control controlMessage;

// Prototipos de funciones
void setup_wifi();
void reconnect_mqtt();
void read_sensors();
void send_mqtt_data();
void setup_sensors();
void sample();
void analyzeSample();
void setNote(int value, int velocity, long duration, int notechannel);
void setControl(int type, int value, int velocity, long duration);
void checkNote();
void checkControl();
int scaleNote(int note, int* scale, int root);
void setLED(byte led, byte brightness);
void updateLEDs();

// ---------- Setup ----------
void setup() {
  if (debugSerial || rawSerial) Serial.begin(115200);
  
  // Inicializar EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Inicializar sensores
  setup_sensors();
  
  // Inicializar MIDI
  if(usbmidi) {
    // usbMIDI.begin(); // Comentado temporalmente
  }
  
  // Configurar pines
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Configurar LEDs
  for(byte i = 0; i < 5; i++) {
    pinMode(leds[i], OUTPUT);
  }
  
  // Light show inicial
  for(byte i = 0; i < 5; i++) {
    setLED(i, ledBrightness[i]);
    delay(200);
  }
  for(byte i = 0; i < 5; i++) {
    setLED(i, 0);
  }
  
  // Configurar WiFi y MQTT
  setup_wifi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setKeepAlive(15);
  
  // Configurar interrupción
  attachInterrupt(digitalPinToInterrupt(interruptPin), sample, RISING);
  
  if(debugSerial) {
    Serial.println("BiodataFeather Integrated v1.0 iniciado");
    Serial.println("Sensores + MIDI + MQTT activos");
  }
}

// ---------- Loop ----------
void loop() {
  currentMillis = millis();
  
  // Mantener conexión MQTT
  if (!mqtt.connected()) {
    reconnect_mqtt();
  }
  mqtt.loop();
  
  // Leer sensores periódicamente
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    read_sensors();
    lastSensorRead = currentMillis;
  }
  
  // Enviar datos MQTT periódicamente
  if (currentMillis - lastMQTTSend >= MQTT_SEND_INTERVAL) {
    send_mqtt_data();
    lastMQTTSend = currentMillis;
  }
  
  // Análisis de muestras biológicas
  if (sampleIndex >= samplesize) {
    analyzeSample();
    sampleIndex = 0;
  }
  
  // Verificar notas y controles MIDI
  checkNote();
  checkControl();
  
  // Actualizar LEDs
  updateLEDs();
  
  delay(1); // Pequeña pausa para estabilidad
}
