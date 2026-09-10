//   ESP32 S3
//Electricity for Progress
// electricityforprogress.com
// store - Biodata - Modules - Custom
// **-----------------------------------------------------------------------------
//ESP32-S3 UDP/RTP AppleMIDI over wifi
//Biodata Sonification pulse input data via interrupt
//Knob and button interface
//LEDs using PWM fading
//ESP32 Feather A13 pin re ads half of battery voltage (4.2 (3.7) -3.2V)
// ~~~NEW~~~
//  * Captive Portal for wifi configuration
//  * Velocity - five levels of control: red-100,yellow-accent(90/120),
//     green-(75,95,120),blue-musical map(value,x,y,50,120),
//     white-fluent map(value,x,y,0,127)
// **-----------------------------------------------------------------------------

/***
1. Static IP Addressing
2. -removed-
3. LED display of wifi status, RTP status, battery status?
  Green Wifi Connected, White RTP connected, Yellow RTP disc conn, Blue Ble conn
  menus
    Red - MIDI channel, Yellow - MIDI Scale, Green - WiFi on/off, Blue - Ble on/off, White - Root note
4. Move key variables to top of .ino (channel, wifi cred, IP, blue/wifi mode)
5. Velocity using CC80, mapping velocity range
6. Enable CC, variable for cc number and mapping range
7. press button, display status LED and usb serial information print

***/

// ============================================================================
// MQTT/InfluxDB Buffer - Includes y configuración
// ============================================================================
#include <WiFiClient.h>
#include <WiFiMulti.h>
#include <esp_system.h>
#include <esp_mac.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"  // Credenciales WiFi, MQTT e InfluxDB

WiFiMulti wifiMulti;
String currentSSID;
bool deviceIdentityReady = false;
int deviceUniq = -1;
String deviceSuffix = "";
String bleDeviceName = "";

void ensureDeviceIdentity();

void ensureDeviceIdentity() {
  if (deviceIdentityReady) {
    return;
  }
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  deviceUniq = 0;
  for (int i = 0; i < 6; i++) {
    deviceUniq += mac[i];
  }
  deviceSuffix = String(deviceUniq);
  bleDeviceName = "BIODATA " + deviceSuffix;
  deviceIdentityReady = true;
}

// Variables externas del buffer MQTT (definidas en MQTTInflux.ino)
extern bool bufferEnabled;
extern PubSubClient mqtt;

// Declaraciones de funciones del buffer MQTT (implementadas en MQTTInflux.ino)
void setupMQTT();
void reconnectMQTT();
void addNoteToBuffer(byte note, byte velocity, int duration, byte channel);
bool sendBufferToInflux();
void checkBufferTimer();
void flushMQTTPayload();
bool hasPendingMQTTData();  // Verificar si hay datos pendientes en el buffer
// ============================================================================

// Declaraciones de funciones ambientales (implementadas en Environmental.ino)
void setupEnvironmentalSensors();
void readEnvironmentalSensors();
void checkEnvironmentalTimer();
// ============================================================================

// Wifi Credentials ahora están en secrets.h
                                       // ~~~~~~~~~~~~~!!!!
                                       //  Set the MIDI Channel of this node
byte channel = 1;                      //
IPAddress local_IP(192, 168, 0, 110);  //use this IP
bool staticIP = false;                 // true;  //toggle for dynamic IP
//static IP

//MIDI Note and Controls
const byte polyphony = 5;  // 1; //mono  // number of notes to track at a given time

//******************************
//set scaled values, sorted array, first element scale length
// indices 0-3 keep previous EEPROM mapping; 4 replaces the broken Indian scale
int scaleChrom[] = { 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
int scaleMinor[] = { 7, 0, 2, 3, 5, 7, 8, 10 };
int scaleMajor[] = { 7, 0, 2, 4, 5, 7, 9, 11 };
int scalePenta[] = { 5, 0, 3, 5, 7, 9 };
int scalePentaMaj[] = { 5, 0, 2, 4, 7, 9 };
int scaleDorian[] = { 7, 0, 2, 3, 5, 7, 9, 10 };
int scaleMixolydian[] = { 7, 0, 2, 4, 5, 7, 9, 10 };
int scaleLydian[] = { 7, 0, 2, 4, 6, 7, 9, 11 };
int scalePhrygian[] = { 7, 0, 1, 3, 5, 7, 8, 10 };
int scaleHarmMinor[] = { 7, 0, 2, 3, 5, 7, 8, 11 };
int scaleWhole[] = { 6, 0, 2, 4, 6, 8, 10 };
int scaleHirajoshi[] = { 5, 0, 2, 3, 7, 8 };

const byte scaleCount = 12;
int *scaleList[] = {
  scaleChrom, scaleMinor, scaleMajor, scalePenta,
  scalePentaMaj, scaleDorian, scaleMixolydian, scaleLydian,
  scalePhrygian, scaleHarmMinor, scaleWhole, scaleHirajoshi
};
const char* scaleName[] = {
  "Chromatic", "Minor", "Major", "Penta Min",
  "Penta Maj", "Dorian", "Mixolydian", "Lydian",
  "Phrygian", "Harm Minor", "Whole Tone", "Hirajoshi"
};

int *scaleSelect = scaleChrom;  //initialize scaling
byte defScale = 3;
byte currentScale = 3;

void applyScale(byte index) {
  if (index >= scaleCount) index = defScale;
  currentScale = index;
  scaleSelect = scaleList[index];
}

int root = 0;  //initialize for root
const char* rootNoteName[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
//*******************************

//Debug and MIDI output Settings ********
byte debugSerial = 1;        //debugging serial messages
byte rawSerial = 0;          // raw biodata stream via serial data
byte serialMIDI = 1;         //write serial data to MIDI hardware output
byte wifiMIDI = 0;           //do all the fancy wifi stuff and RTP MIDI over AppleMIDI
byte bleMIDI = 0;            //bluetooth midi
byte usbmidi = 1;            //usb MIDI connection - ESP32 v3.3.2 native TinyUSB
byte midiMode = 1;           //change mode for serial, ble, wifi, usb
byte wifiActive = 1;         //turn the wifi on and off, needed for wifiMIDI apparently
byte bleActive = 0;          //toggle for connected disconnected?  prob not needed, use library
byte midiControl = channel;  // 1; //use channel 16 to recieve parameter changes? fancy! or recv on base channel...hmm
//setting channel to 11 or 12 often helps simply computer midi routing setups
int noteMin = 36;         //C2  - keyboard note minimum
int noteMax = 96;         //C7  - keyboard note maximum
byte controlNumber = 80;  //set to mappable control, low values may interfere with other soft synth controls!!
unsigned long rawSerialTime = 0;
int rawSerialDelay = 0;
// **************************************

#include <EEPROM.h>
#define EEPROM_SIZE 6  // scaleindex, midi channel, wifi, bluetooth, key, root


// I/O Pin declarations
int buttonPin = 13;
int potPin = A2;               //A2 ESP32 and A0 ESP8266
const byte interruptPin = 12;  //galvanometer input

//leds
byte leds[5] = { 18, 17, 8, 36, 35 };               //ESP32-S3 pins  // ESP32Huzzah -- { 26,25,4,5,18 };
byte ledBrightness[5] = { 50, 110, 120, 120, 60 };  //{185,255,255,255,195};
byte maxBrightness = 60;
bool blinkToggle = 0;
unsigned long blinkTime = 0;

//USB MIDI - ESP32 v3.3.2 Native Implementation
#include "USB.h"
#include "USBMIDI.h"
USBMIDI usbMIDI;


//Bluetooth Configuration
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

BLECharacteristic *pCharacteristic = nullptr;
BLEServer *bleServer = nullptr;
bool deviceConnected = false;
bool bleConnectFlashPending = false;
unsigned long bleConnectFlashOffAt = 0;

uint8_t midiPacket[] = {
  0x80,  // header
  0x80,  // timestamp, not implemented (same as v10; hosts accept this)
  0x00,  // status
  0x3c,  // 0x3c == 60 == middle c
  0x00   // velocity
};

void sendBleMidi(byte status, byte data1, byte data2) {
  if (!bleMIDI || !deviceConnected || pCharacteristic == nullptr) {
    return;
  }
  midiPacket[0] = 0x80;
  midiPacket[1] = 0x80;
  midiPacket[2] = status;
  midiPacket[3] = data1 & 0x7F;
  midiPacket[4] = data2 & 0x7F;
  pCharacteristic->setValue(midiPacket, 5);
  pCharacteristic->notify();
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    bleConnectFlashPending = true;  // 1 s blue flash, handled in loop()
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    bleConnectFlashPending = false;
    bleConnectFlashOffAt = 0;
    pServer->startAdvertising();  // keep advertising after disconnect
  }
};


//fading leds
/*
 * pinNumber, espPWMchannel, maxBright,
 * currentLevel, destinationLevel, startTime, 
 * duration, stepSize, stepTime, isRunning
 * set stepSize as rate of fade based on maxBrightness and duration
 *      stepSize = duration/destinationLevel
 * if isRunning
 *   if currTime-startTime<duration //fade running
 *     if currTime=stepTime>stepSize
 *       stepTime=currTime
 *       if curr<dest, curr++
 *       if curr>dest, cur--
 *   else isRunning = FALSE //fade ended
 */
class samFader {

public:
  byte pinNumber;
  byte espPWMchannel;
  int maxBright;
  int currentLevel = 0;
  int destinationLevel = 0;
  unsigned long startTime;  //time at which a fade begins
  int duration = 0;
  unsigned long stepSize;  //computed duration across brightness
  unsigned long stepTime;  //last time a step was called
  bool isRunning = 0;

  samFader(byte pin, byte pwmChannel, byte maxB) {
    pinNumber = pin;
    espPWMchannel = pwmChannel;
    maxBright = maxB;
  }

  void Set(int dest, int dur) {
    startTime = millis();
    destinationLevel = dest;
    duration = dur;
    stepTime = 0;
    int difference = abs(destinationLevel - currentLevel);
    stepSize = duration / (difference + 1);
    isRunning = 1;
    if (dur == 0) {  // do it right away!
                     //  ledcWrite(espPWMchannel, dest); //updated for esp 3.0
      ledcWrite(pinNumber, dest);
      isRunning = 0;
    }
  }

  void Update() {
    if (isRunning) {
      if (stepTime + stepSize < millis()) {
        //update to the next level
        if (currentLevel > destinationLevel) currentLevel--;
        if (currentLevel < destinationLevel) currentLevel++;
        //update variables for tracking
        stepTime = millis();
        //write brightness to LED
        ledcWrite(pinNumber, currentLevel);
      }
      if (startTime + duration < millis()) {
        ledcWrite(pinNumber, destinationLevel);
        isRunning = 0;
      }
    }
  }

  void Setup(byte chan) {
    //ledcSetup(chan,5000,8);
    //ledcAttachPin(pinNumber,chan);
    //espressif v3.0 has breaking changes (and fades) for ledc, friends don't do this to eachother!
    ledcAttach(pinNumber, 5000, 8);
  }
};

samFader ledFaders[] = { samFader(leds[0], 0, ledBrightness[0]),
                         samFader(leds[1], 1, ledBrightness[1]),
                         samFader(leds[2], 2, ledBrightness[2]),
                         samFader(leds[3], 3, ledBrightness[3]),
                         samFader(leds[4], 4, ledBrightness[4]) };


class samButton {
public:

  unsigned int doubleClickTime = 300;  //milliseconds within which a double click event could occur
  unsigned int debounce = 35;          //debounce time
  bool changed = false;                //short lived indicator that a change happened, will see after an update call

  samButton(byte buttonPin, bool pUp);  //constructor

  void begin();                 // initialize button pin with pullup
  bool update();                // update button and led
  bool read();                  // read current button state 'raw' value
  unsigned long changedTime();  // returns last changed time
  bool pressed();               // is the button currently pressed?
  bool wasPressed();            // was the button just depressed?
  bool wasReleased();           // was the button just released?
  bool longPress();             // if(read() && currentMillis - pressStart > _longTime)
  bool doubleClick();           // if(_clickCount == 2)

private:
  unsigned int _buttonPin;
  unsigned int _buttonIndex = 0;
  bool _pullup = 1;               // resistor pullup = 1
  unsigned int _clickTime = 100;  //time to detect true click
  unsigned int _longTime = 1000;  //time to detect long press
  unsigned int _clickCount;       // number of times button was clicked within a double click time
  bool _state = 0;
  bool _prevState = 0;
  unsigned long _changeTime = 0;    //denotes change after toggle or update to button state
  unsigned long _prevDebounce = 0;  // previous debounce toggle in real time
};

//begin SamButton
//****************
// Simple Button debouncing
//****************
//#include "samButton.h"

samButton::samButton(byte buttonPin, bool pUp) {
  _buttonPin = buttonPin;
  _pullup = pUp;
}

void samButton::begin() {
  if (_pullup == 1) pinMode(_buttonPin, INPUT_PULLUP);  //setup button input
  else pinMode(_buttonPin, INPUT);                      // no pullup
  _state = !digitalRead(_buttonPin);
  //set button prev state
  _prevState = _state;
  //set button time
  _prevDebounce = millis();
  //change time is current time
  _changeTime = _prevDebounce;
  //change is false
  changed = 0;
}

bool samButton::update() {
  unsigned long milli = millis();  //get current time

  //bool reading = MPR121.getTouchData();  //
  bool reading = !digitalRead(_buttonPin);  //get current reading
  if (reading != _prevState) {              // if current reading is change from last secure state
    _prevState = reading;                   // reset bouncing reading
    _prevDebounce = milli;                  // update bouncing time
  }

  if (milli - _prevDebounce > debounce) {  //evaulate debounce time
    _prevDebounce = milli;                 //reset time
    if (reading != _state) {               // debounced and changed
      changed = 1;
      _state = reading;
      _prevState = _state;
      _changeTime = milli;
    }
  } else changed = 0;  //has not changed or still under debounce time
  return _state;
}

bool samButton::read() {
  return !digitalRead(_buttonPin);
  ;
}


bool samButton::pressed() {  //returns true if the button is pressed currently
  return _state;
}

unsigned long samButton::changedTime() {  // used to determine how long button has been in current state
  return _changeTime;
}

bool samButton::wasPressed() {
  return changed && _state;  // if both just changed and button is pressed
}

bool samButton::wasReleased() {
  return changed && !_state;
}
//end SamButton

//declare button
samButton button(buttonPin, true);

//Timing and tracking
unsigned long currentMillis = 0;
unsigned long prevMillis = 0;


//****** sample size sets the 'grain' of the detector
// a larger size will smooth over small variations
// a smaller size will excentuate small changes
const byte samplesize = 10;             //set sample array size
const byte analysize = samplesize - 1;  //trim for analysis array,

volatile unsigned long microseconds;  //sampling timer
volatile byte sampleIndex = 0;
volatile unsigned long samples[samplesize];

float threshold = 1.71;  //threshold multiplier
float threshMin = 1.61;  //scaling threshold min
float threshMax = 4.01;  //scaling threshold max
float prevThreshold = 0;


//I did this before using the other MIDI libraries ....
typedef struct _MIDImessage {  //build structure for Note and Control MIDImessages
  unsigned int type;
  int value;
  int velocity;
  long duration;
  long period;
  int channel;
} MIDImessage;
MIDImessage noteArray[polyphony];  //manage MIDImessage data as an array with size polyphony
int noteIndex = 0;
MIDImessage controlMessage;  //manage MIDImessage data for Control Message (CV out)



#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

//setups for each MIDI type, provide led display output
void setupSerialMIDI() {
  if (debugSerial) Serial.println("MIDI set on Serial1 31250");
  const int MIDI_TX_PIN = 39;  // minijack PCB Feather ESP32-S3 (same as v10 default TX)
  Serial1.begin(31250, SERIAL_8N1, -1, MIDI_TX_PIN);
}

void checkKnob() {
  //float knobValue
  bool bigChange = 0;
  threshold = analogRead(potPin);

  if (abs(prevThreshold - threshold) > 200) {
    bigChange = 1;
  }


  prevThreshold = threshold;  //remember last value
  //set threshold to knobValue mapping
  threshold = mapfloat(threshold, 0, 4095, threshMin, threshMax);
}

//provide float map function
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#define SerialMon Serial
#define APPLEMIDI_DEBUG SerialMon
//#include "AppleMidi.h"
//https://github.com/lathoub/Arduino-AppleMIDI-Library
#include <AppleMIDI.h>

bool isConnected = false;

//APPLEMIDI_CREATE_INSTANCE(WiFiUDP, AppleMIDI); // see definition in AppleMidi_Defs.h
APPLEMIDI_CREATE_DEFAULTSESSION_INSTANCE();

void setupWifi() {

  //connect with static IP

  IPAddress gateway(192, 168, 0, 1);
  IPAddress dns(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  if (staticIP) {
    if (!WiFi.config(local_IP, dns, gateway, subnet)) {
      if (debugSerial) Serial.print(F("Static IP config Failure"));
    }
  }

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
#if defined(WIFI_SSID_2) && defined(WIFI_PASSWORD_2)
  if (WIFI_SSID_2[0] != '\0') {
    wifiMulti.addAP(WIFI_SSID_2, WIFI_PASSWORD_2);
  }
#endif
#if defined(WIFI_SSID_3) && defined(WIFI_PASSWORD_3)
  if (WIFI_SSID_3[0] != '\0') {
    wifiMulti.addAP(WIFI_SSID_3, WIFI_PASSWORD_3);
  }
#endif

  if (debugSerial) {
    Serial.println(F("Intentando conexión WiFi (hasta 15 s)..."));
  }

  //could get 'stuck' here if we can't connect ... hmm..
  //timeout after some duration  to allow Serial MIDI or
  //combo BLE (who would do that?)
  //but it doesn't seem to get stuck .. at home duh!
  bool ledToggle = 1;
  unsigned long wifiMillis = millis();
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    if (debugSerial) Serial.print(F("."));
    //flash LED on and off toggle
    if (ledToggle) {
      ledFaders[0].Set(ledFaders[0].maxBright, 0);
    } else ledFaders[0].Set(0, 0);
    ledToggle = !ledToggle;
    ledFaders[0].Update();
    if (wifiMillis + 15000 < millis()) break;
  }

  //was wifi connectionsuccessfuL?
  if (WiFi.status() == WL_CONNECTED) {
    currentSSID = WiFi.SSID();
    if (debugSerial) Serial.println(F("WiFi connected"));
    //solid LED
    ledFaders[0].Set(0, 0);                       //turn off Red
    ledFaders[2].Set(ledFaders[2].maxBright, 0);  //turn on Green
    delay(1000);
    ledFaders[2].Set(0, 4000);  //fade out green LED
  } else {
    currentSSID = "";
    if (debugSerial) Serial.println(F("WiFi NOT connected"));
    ledFaders[0].Set(ledFaders[0].maxBright, 0);  //turn on Red
  }
  ensureDeviceIdentity();
  byte mac[6];
  WiFi.macAddress(mac);
  String macAddr = String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  if (debugSerial) {
    Serial.print("MAC Address: ");
    Serial.println(macAddr);
    Serial.print("Biodata ");
    Serial.println(deviceUniq);
    Serial.print(F("IP address is "));
    Serial.println(WiFi.localIP());
  }
  // Create a session and wait for a remote host to connect to us
  //  AppleMIDI.begin("ESP32_MIDI");
  MIDI.begin();
  //isConnected
  AppleMIDI.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t &ssrc, const char *name) {
    isConnected = 1;
    //    DBG(F("Connected to session"), ssrc, name);
    Serial.print("Connected to session ");
    Serial.println(name);
    ledFaders[4].Set(ledFaders[4].maxBright, 0);  // turn on white LED;
    delay(1000);
    ledFaders[4].Set(0, 0);
  });
  AppleMIDI.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t &ssrc) {
    isConnected = 0;
    //    DBG(F("Disconnected"), ssrc);
    Serial.println("AppleMIDI Disconnected");
    ledFaders[1].Set(ledFaders[1].maxBright, 0);  // turn on white LED;
    delay(1000);
    ledFaders[1].Set(0, 0);
  });
}

void bleStop() {
  deviceConnected = false;
  pCharacteristic = nullptr;
  bleServer = nullptr;
  if (BLEDevice::getInitialized()) {
    BLEDevice::deinit(false);  // false = allow BLEDevice::init() later
  }
}

void bleSetup() {
  ensureDeviceIdentity();

  // BLEDevice::init() only runs once unless we deinit first
  if (BLEDevice::getInitialized()) {
    bleStop();
    delay(50);
  }

  BLEDevice::init(bleDeviceName.c_str());

  if (debugSerial) {
    Serial.print("BiodataBLE ");
    Serial.println(bleDeviceName);
  }
  //bluetooth notification
  ledFaders[3].Set(ledFaders[3].maxBright, 0);
  delay(1000);

  // Create the BLE Server
  bleServer = BLEDevice::createServer();
  static MyServerCallbacks bleCallbacks;
  bleServer->setCallbacks(&bleCallbacks);

  // Create the BLE Service
  BLEService *pService = bleServer->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    BLEUUID(CHARACTERISTIC_UUID),
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Same advertising as v10 (working BLE MIDI): library builds the packet.
  // Custom scan-response data overwrote that packet and hid the MIDI UUID.
  BLEAdvertising *pAdvertising = bleServer->getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();
}


void showBinaryLeds(int value) {
  for (byte i = 0; i < 5; i++) ledFaders[i].Set(0, 0);
  for (byte i = 0; i < 5; i++) {
    if (value % 2) ledFaders[i].Set(ledFaders[i].maxBright, 0);
    value = value / 2;
  }
}

// Helper: rangos de potenciometro equitativos (Arduino map deja el max casi inalcanzable)
static int mapMenuIndex(int knob, int count) {
  if (count <= 1) return 0;
  long v = map((long)knob, 0L, 4095L, 0L, (long)count);
  if (v >= count) v = count - 1;
  if (v < 0) v = 0;
  return (int)v;
}

static void saveMenuSelection(byte menu, byte modeValue) {
  if (menu == 0) {
    applyScale(modeValue);
    EEPROM.write(0, currentScale);
    if (debugSerial) {
      Serial.print("MIDI Scale ");
      Serial.print(currentScale);
      Serial.print(" ");
      Serial.println(scaleName[currentScale]);
    }
  } else if (menu == 1) {
    if (modeValue < 1) modeValue = 1;
    if (modeValue > 16) modeValue = 16;
    channel = modeValue;
    EEPROM.write(1, channel);
    if (debugSerial) {
      Serial.print("Channel ");
      Serial.println(channel);
    }
  } else if (menu == 2) {
    // Solo aplicar si cambia el estado (evita re-setupWifi en timeout)
    if (modeValue == 0 && wifiMIDI != 0) {
      if (debugSerial) Serial.println("Wifi Shutdown ");
      bufferEnabled = false;
      WiFi.disconnect(true);
      delay(1);
      WiFi.mode(WIFI_OFF);
      delay(1);
      wifiMIDI = 0;
      EEPROM.write(2, 0);
    } else if (modeValue == 1 && wifiMIDI != 1) {
      if (debugSerial) Serial.println("Wifi Power On");
      wifiMIDI = 1;
      EEPROM.write(2, 1);
      setupEnvironmentalSensors();  // sonda unica; no-op si ya se probo sin hardware
      setupWifi();
      setupMQTT();
      bufferEnabled = true;
    } else {
      EEPROM.write(2, wifiMIDI ? 1 : 0);
    }
  } else if (menu == 3) {
    if (modeValue == 0 && bleMIDI != 0) {
      bleMIDI = 0;
      bleStop();
      EEPROM.write(3, 0);
      if (debugSerial) Serial.println("BLE Off");
    } else if (modeValue == 1 && bleMIDI != 1) {
      bleMIDI = 1;
      EEPROM.write(3, 1);
      if (!BLEDevice::getInitialized()) {
        bleSetup();
      } else {
        ledFaders[menu].Set(ledFaders[menu].maxBright, 0);
        delay(1000);
      }
      if (debugSerial) Serial.println("BLE On");
    } else {
      EEPROM.write(3, bleMIDI ? 1 : 0);
    }
    ledFaders[menu].Set(0, 700);
  } else if (menu == 4) {
    if (modeValue > 11) modeValue = 11;
    root = modeValue;
    EEPROM.write(5, root);
    if (debugSerial) {
      Serial.print("Root ");
      Serial.print(root);
      Serial.print(" ");
      Serial.println(rootNoteName[root]);
    }
  }

  EEPROM.commit();
  if (debugSerial) Serial.println("settings Saved");
}

void checkButton() {
  byte modeValue = 0;
  button.update();

  if (button.wasReleased()) {
    if (debugSerial) {
      Serial.println("---***---***---ButtonClick***---***---***");
      ensureDeviceIdentity();
      byte mac[6];
      WiFi.macAddress(mac);
      String macAddr = String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
      Serial.print("Biodata ");
      Serial.println(deviceUniq);
      Serial.print("MIDI Channel ");
      Serial.println(channel);
      Serial.print("ScaleIndex ");
      Serial.print(currentScale);
      Serial.print(" ");
      Serial.println(scaleName[currentScale]);
      Serial.print("Root ");
      Serial.print(root);
      Serial.print(" ");
      Serial.println(rootNoteName[root]);
      Serial.print("Threshold ");
      Serial.println(threshold);
      Serial.print("MAC Address: ");
      Serial.println(macAddr);

      if (usbmidi) Serial.println("USB MIDI On");
      else Serial.println("USB MIDI Off");
      if (bleMIDI) {
        Serial.print("Bluetooth On ");
        if (deviceConnected) Serial.println("Bluetooth Connected");
        else Serial.println("Bluetooth DisConnected");
      } else Serial.println("Bluetooth Off");

      if (wifiMIDI) {
        Serial.print("SSID configurado/activo: ");
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println(WiFi.SSID());
        } else if (currentSSID.length()) {
          Serial.println(currentSSID);
        } else {
          Serial.println(WIFI_SSID);
        }
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Wifi Connected");
        } else Serial.println("WiFi Not Connected");

        Serial.print("RSS Signal level: ");
        Serial.println(WiFi.RSSI());
        if (isConnected) Serial.println(F("RTP MIDI Connected!"));
        else Serial.println("RTP MIDI Not Connected");
        Serial.println();
        Serial.print(F("IP address is "));
        Serial.println(WiFi.localIP());
      } else Serial.println("Wifi Off");
    }

    int knobValue = analogRead(potPin);
    int prevKnob = knobValue;
    unsigned long menuTimer = millis();
    int menu = 0;
    const unsigned long MENU_PICK_MS = 10000;
    const unsigned long MENU_EDIT_MS = 20000;

    while (menuTimer + MENU_PICK_MS > millis()) {
      knobValue = analogRead(potPin);
      // Girar el pot renueva el tiempo (antes estaba comentado → timeout sin guardar)
      if (abs(knobValue - prevKnob) > 45) {
        menuTimer = millis();
      }
      prevKnob = knobValue;

      // 5 menús con rangos equitativos (antes Root solo con ADC≈4095)
      menu = mapMenuIndex(knobValue, 5);

      for (byte i = 0; i < 5; i++) { ledFaders[i].Set(0, 0); }
      if (blinkTime < millis()) {
        blinkToggle = !blinkToggle;
        blinkTime = millis() + 120;
      }
      if (blinkToggle) ledFaders[menu].Set(ledFaders[menu].maxBright, 0);

      button.update();
      modeValue = 0;

      if (button.wasReleased()) {
        if (debugSerial) {
          Serial.print("Enter Menu ");
          Serial.print(menu);
          if (menu == 0) Serial.println(" Scale");
          if (menu == 1) Serial.println(" Channel");
          if (menu == 2) Serial.println(" Wifi");
          if (menu == 3) Serial.println(" Bluetooth");
          if (menu == 4) Serial.println(" Root");
        }
        menuTimer = millis();
        // El pot es el mismo que el umbral: al entrar hay que mostrar el valor
        // guardado, no lo que diga la posición actual del pot.
        const int entryKnob = analogRead(potPin);
        prevKnob = entryKnob;
        bool knobEngaged = false;  // true cuando el usuario mueve el pot a proposito

        if (menu == 0) modeValue = currentScale;
        else if (menu == 1) modeValue = channel;
        else if (menu == 2) modeValue = wifiMIDI ? 1 : 0;
        else if (menu == 3) modeValue = bleMIDI ? 1 : 0;
        else if (menu == 4) modeValue = root;

        bool saved = false;
        while (menuTimer + MENU_EDIT_MS > millis()) {
          button.update();
          knobValue = analogRead(potPin);

          // Solo empezar a leer el pot tras un movimiento claro desde la entrada
          if (!knobEngaged && abs(knobValue - entryKnob) > 80) {
            knobEngaged = true;
          }
          if (knobEngaged && abs(knobValue - prevKnob) > 45) {
            menuTimer = millis();
          }
          prevKnob = knobValue;

          for (byte i = 0; i < 5; i++) { ledFaders[i].Set(0, 0); }

          if (menu == 0) {
            if (knobEngaged) {
              modeValue = mapMenuIndex(knobValue, scaleCount);
              applyScale(modeValue);
            }
            showBinaryLeds(modeValue);
          } else if (menu == 1) {
            if (knobEngaged) {
              modeValue = mapMenuIndex(knobValue, 16) + 1;  // 1..16
              channel = modeValue;
            }
            showBinaryLeds(modeValue);
          } else if (menu == 2) {
            if (knobEngaged) {
              modeValue = (knobValue > 2047) ? 1 : 0;
            }
            if (blinkTime < millis()) {
              blinkToggle = !blinkToggle;
              blinkTime = millis() + 150;
            }
            byte statusLed = modeValue * 4;
            if (blinkToggle) ledFaders[statusLed].Set(ledFaders[statusLed].maxBright, 0);
            ledFaders[menu].Set(ledFaders[menu].maxBright, 0);
          } else if (menu == 3) {
            if (knobEngaged) {
              modeValue = (knobValue > 2047) ? 1 : 0;
            }
            if (blinkTime < millis()) {
              blinkToggle = !blinkToggle;
              blinkTime = millis() + 150;
            }
            byte statusLed = modeValue * 4;
            if (blinkToggle) ledFaders[statusLed].Set(ledFaders[statusLed].maxBright, 0);
            ledFaders[menu].Set(ledFaders[menu].maxBright, 0);
          } else if (menu == 4) {
            if (knobEngaged) {
              modeValue = mapMenuIndex(knobValue, 12);  // 0..11
              root = modeValue;
            }
            showBinaryLeds(modeValue);
            ledFaders[menu].Set(ledFaders[menu].maxBright, 0);
          }

          if (button.wasReleased()) {
            for (byte i = 0; i < 5; i++) { ledFaders[i].Set(0, 0); }
            ledFaders[menu].Set(ledFaders[menu].maxBright, 0);
            delay(75);
            ledFaders[menu].Set(0, 0);
            delay(75);
            ledFaders[menu].Set(ledFaders[menu].maxBright, 0);
            delay(75);
            ledFaders[menu].Set(0, 0);
            delay(75);
            ledFaders[menu].Set(ledFaders[menu].maxBright, 0);
            delay(75);
            ledFaders[menu].Set(0, 0);
            for (byte i = 0; i < 5; i++) { ledFaders[i].Set(0, 0); }

            saveMenuSelection(menu, modeValue);
            saved = true;
            button.update();
            break;
          }
        }

        // Timeout del submenú: persistir el valor que se estaba previsualizando
        // (antes se perdía al reiniciar porque solo vivía en RAM)
        if (!saved) {
          if (debugSerial) Serial.println("Menu timeout — guardando valor actual");
          saveMenuSelection(menu, modeValue);
        }

        for (byte i = 0; i < 5; i++) { ledFaders[i].Set(0, 0); }
        return;
      }
    }

    for (byte i = 0; i < 5; i++) { ledFaders[i].Set(0, 0); }
  }
}
