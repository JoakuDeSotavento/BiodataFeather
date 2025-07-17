/***************************************************
  This is an example for the LTR329 light sensor that reads both channels
  and demonstrates how to set gain and check data validity

  Designed specifically to work with the LTR-329 light sensor from Adafruit
  ----> https://www.adafruit.com/product/5591

  These sensors use I2C to communicate, 2 pins are required to
  interface
 ****************************************************/
// 
#include "Adafruit_LTR329_LTR303.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
// BME688
#include "DFRobot_BME68x.h"
#include "Wire.h"
#include <PubSubClient.h>
#include "secrets.h"

WiFiClient espClient;               // Cliente WiFi
PubSubClient client(espClient);     // Cliente MQTT sobre WiFi

// InfluxDB v2 credentials
const char* influxURL = "https://db.sinfoniabiotica.xyz:443/api/v2/write?org=MolinoLab&bucket=biodata&precision=s";
const char* influxToken = "0b2BkqoPhbVEg3yXNhYC09odJmLvSK8RlrjGndZiAS5wEeKqqNiG7ZVeP6U2MoRg86UsFgHTwBpq1_Ls4TsB9A==";

Adafruit_LTR329 ltr = Adafruit_LTR329();

/*use an accurate altitude to calibrate sea level air pressure*/
#define CALIBRATE_PRESSURE

DFRobot_BME68x_I2C bme(0x77);  //0x77 I2C address

float seaLevel;

void setup() {
  uint8_t rslt = 1;
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Adafruit LTR-329 advanced test + BME688");
  delay(1000);
  Serial.println();

  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);

  if ( ! ltr.begin() ) {
    Serial.println("Couldn't find LTR sensor!");
    while (1) delay(10);
  }
  Serial.println("Found LTR sensor!");

  while(rslt != 0) {
    rslt = bme.begin();
    if(rslt != 0) {
      Serial.println("bme begin failure");
      delay(2000);
    }
  }
  Serial.println("bme begin successful");
  #ifdef CALIBRATE_PRESSURE
  bme.startConvert();
  delay(1000);
  bme.update();
  /*You can use an accurate altitude to calibrate sea level air pressure. 
   *And then use this calibrated sea level pressure as a reference to obtain the calibrated altitude.
   *In this case,525.0m is chendu accurate altitude.
   */
  seaLevel = bme.readSeaLevel(525.0);
  Serial.print("seaLevel :");
  Serial.println(seaLevel);
  #endif
  // At initialization, the default heating layer target temperature is 320 and the duration is 150ms. The heating layer temperature is modified here.
  bool res = bme.setGasHeater(360, 100);
  
  Serial.print("Set the target temperature of the heating layer and the heating time: ");
  if(res == true){
    Serial.println("set successful!");
  }else{
    Serial.println("set failure!");
  }

  ltr.setGain(LTR3XX_GAIN_2);
  Serial.print("Gain : ");
  switch (ltr.getGain()) {
    case LTR3XX_GAIN_1: Serial.println(1); break;
    case LTR3XX_GAIN_2: Serial.println(2); break;
    case LTR3XX_GAIN_4: Serial.println(4); break;
    case LTR3XX_GAIN_8: Serial.println(8); break;
    case LTR3XX_GAIN_48: Serial.println(48); break;
    case LTR3XX_GAIN_96: Serial.println(96); break;
  }

  ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
  Serial.print("Integration Time (ms): ");
  switch (ltr.getIntegrationTime()) {
    case LTR3XX_INTEGTIME_50: Serial.println(50); break;
    case LTR3XX_INTEGTIME_100: Serial.println(100); break;
    case LTR3XX_INTEGTIME_150: Serial.println(150); break;
    case LTR3XX_INTEGTIME_200: Serial.println(200); break;
    case LTR3XX_INTEGTIME_250: Serial.println(250); break;
    case LTR3XX_INTEGTIME_300: Serial.println(300); break;
    case LTR3XX_INTEGTIME_350: Serial.println(350); break;
    case LTR3XX_INTEGTIME_400: Serial.println(400); break;
  }

  ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
  Serial.print("Measurement Rate (ms): ");
  switch (ltr.getMeasurementRate()) {
    case LTR3XX_MEASRATE_50: Serial.println(50); break;
    case LTR3XX_MEASRATE_100: Serial.println(100); break;
    case LTR3XX_MEASRATE_200: Serial.println(200); break;
    case LTR3XX_MEASRATE_500: Serial.println(500); break;
    case LTR3XX_MEASRATE_1000: Serial.println(1000); break;
    case LTR3XX_MEASRATE_2000: Serial.println(2000); break;
  }
}

void loop() {
  bool valid;
  uint16_t visible_plus_ir, infrared;

  bme.startConvert();
  delay(1000);
  bme.update();

  float temperatura = bme.readTemperature() / 100.0;
  float presion = bme.readPressure();
  float humedad = bme.readHumidity() / 1000.0;
  float gas = bme.readGasResistance();
  float altitud = bme.readAltitude();
  float altitudCalibrada = bme.readCalibratedAltitude(seaLevel);

  Serial.println();
  Serial.print("temperature(C) :"); Serial.println(temperatura);
  Serial.print("pressure(Pa) :"); Serial.println(presion);
  Serial.print("humidity(%rh) :"); Serial.println(humedad);
  Serial.print("gas resistance(ohm) :"); Serial.println(gas);
  Serial.print("altitude(m) :"); Serial.println(altitud);
  Serial.print("calibrated altitude(m) :"); Serial.println(altitudCalibrada);

  if (ltr.newDataAvailable()) {
    valid = ltr.readBothChannels(visible_plus_ir, infrared);
    if (valid) {
      Serial.print("CH0 Visible + IR: ");
      Serial.print(visible_plus_ir);
      Serial.print("\t\tCH1 Infrared: ");
      Serial.println(infrared);
    }
  }

  // Crear línea InfluxDB (medición: "sensores")
  String data = "sensores,sensor=esp32 ";
  data += "temperatura=" + String(temperatura, 2) + ",";
  data += "presion=" + String(presion) + ",";
  data += "humedad=" + String(humedad, 2) + ",";
  data += "gas=" + String(gas) + ",";
  data += "altitud=" + String(altitud, 2) + ",";
  data += "altitudCalibrada=" + String(altitudCalibrada, 2) + ",";
  data += "visible_ir=" + String(visible_plus_ir) + ",";
  data += "infrarrojo=" + String(infrared);

  sendToInflux(data);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  String payload = "{";
    payload += "\"temperature\":" + String(temperatura, 2) + ",";
    payload += "\"presion\":" + String(presion) + ",";
    payload += "\"humedad\":" + String(humedad, 2) + ",";
    payload += "\"gas\":" + String(gas) + ",";
    payload += "\"altitud\":" + String(altitud, 2) + ",";
    payload += "\"altitudCalibrada\":" + String(altitudCalibrada, 2) + ",";
    payload += "\"visible_ir\":" + String(visible_plus_ir) + ",";
    payload += "\"infrarrojo\":" + String(infrared);
    payload += "}";

    // 📬 Enviar a tópico específico por sensor
    String topic = String(MQTT_BASE_TOPIC) + "/" + SENSOR_ID;
    client.publish(topic.c_str(), payload.c_str());

    Serial.print("Publicado en ");
    Serial.println(topic);
    Serial.println(payload);

  delay(60000); // esperar 1 minuto antes de la próxima lectura
}

void sendToInflux(String line) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // ⚠️ Desactiva validación de certificado (no recomendado en producción)

    HTTPClient https;
    if (https.begin(client, influxURL)) {
      https.addHeader("Authorization", "Token " + String(influxToken));
      https.addHeader("Content-Type", "text/plain");

      int response = https.POST(line);
      Serial.print("InfluxDB response: ");
      Serial.println(response);
      if (response > 0) {
        Serial.println(https.getString());
      }
      https.end();
    } else {
      Serial.println("Error al iniciar conexión HTTPS");
    }
  } else {
    Serial.println("WiFi no conectado, no se pueden enviar datos.");
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.printf("Conectando a %s\n", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Reconexión al broker
  while (!client.connected()) {
    Serial.print("Conectando a MQTT...");
    if (client.connect(SENSOR_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("conectado");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando en 5 segundos");
      delay(5000);
    }
  }
}