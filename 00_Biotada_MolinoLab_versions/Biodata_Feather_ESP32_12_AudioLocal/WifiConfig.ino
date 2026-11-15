// Configuración WiFi/OSC en NVS y portal SoftAP si la STA no conecta.
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

static const char *NET_NVS_NS = "biodata";
static const char *PORTAL_HOST = "biodata.sinfoniabiotica.xyz";
static const unsigned long PORTAL_TIMEOUT_MS = 10UL * 60UL * 1000UL;
static const IPAddress PORTAL_IP(192, 168, 4, 1);
static const IPAddress PORTAL_MASK(255, 255, 255, 0);

static WebServer configServer(80);
static DNSServer configDns;
static bool nvsApAdded = false;
static bool secretsApsAdded = false;

String htmlEscape(const String &in) {
  String out;
  out.reserve(in.length() + 8);
  for (unsigned int i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '&') out += F("&amp;");
    else if (c == '<') out += F("&lt;");
    else if (c == '"') out += F("&quot;");
    else out += c;
  }
  return out;
}

void blinkRed(bool on) {
  ledFaders[0].Set(on ? ledFaders[0].maxBright : 0, 0);
  ledFaders[0].Update();
}

bool runMultiUntil(uint32_t timeoutMs) {
  unsigned long start = millis();
  bool ledOn = true;
  while (millis() - start < timeoutMs) {
    if (wifiMulti.run() == WL_CONNECTED) return true;
    delay(500);
    if (debugSerial) Serial.print(F("."));
    ledOn = !ledOn;
    blinkRed(ledOn);
  }
  return WiFi.status() == WL_CONNECTED;
}

void addNvsAp() {
  if (nvsApAdded || !netConfig.fromNvs || !netConfig.ssid.length()) return;
  nvsApAdded = true;
  wifiMulti.addAP(netConfig.ssid.c_str(), netConfig.pass.c_str());
}

void addSecretsAps() {
  if (secretsApsAdded) return;
  secretsApsAdded = true;
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
}

void loadNetConfig() {
  if (netConfigLoaded) return;

  netConfig.ssid = "";
  netConfig.pass = "";
  netConfig.destMode = DEST_ONLINE;
  netConfig.oscIP = OSC_IP_DEFAULT;
  netConfig.oscPort = OSC_PORT_DEFAULT;
  netConfig.payloadMode = PAYLOAD_BOTH;
  netConfig.fromNvs = false;

  Preferences prefs;
  prefs.begin(NET_NVS_NS, true);
  String ssid = prefs.getString("ssid", "");
  if (ssid.length()) {
    netConfig.ssid = ssid;
    netConfig.pass = prefs.getString("pass", "");
    netConfig.fromNvs = true;
  }
  uint8_t dest = prefs.getUChar("dest", DEST_ONLINE);
  uint8_t pay = prefs.getUChar("pay", PAYLOAD_BOTH);
  String ip = prefs.getString("oscip", "");
  uint16_t port = prefs.getUShort("oscport", 0);
  prefs.end();

  if (dest <= DEST_LOCAL) netConfig.destMode = dest;
  if (pay <= PAYLOAD_BOTH) netConfig.payloadMode = pay;
  if (ip.length()) netConfig.oscIP = ip;
  if (port) netConfig.oscPort = port;
  netConfigLoaded = true;
}

void saveNetConfig() {
  Preferences prefs;
  prefs.begin(NET_NVS_NS, false);
  prefs.putString("ssid", netConfig.ssid);
  prefs.putString("pass", netConfig.pass);
  prefs.putUChar("dest", netConfig.destMode);
  prefs.putUChar("pay", netConfig.payloadMode);
  prefs.putString("oscip", netConfig.oscIP);
  prefs.putUShort("oscport", netConfig.oscPort);
  prefs.end();
  netConfig.fromNvs = netConfig.ssid.length() > 0;
  netConfigLoaded = true;
}

void clearNetConfig() {
  Preferences prefs;
  prefs.begin(NET_NVS_NS, false);
  prefs.clear();
  prefs.end();
  netConfigLoaded = false;
  nvsApAdded = false;
  loadNetConfig();
}

bool wifiDestIsLocal() {
  loadNetConfig();
  return netConfig.destMode == DEST_LOCAL;
}

bool oscSendRawEnabled() {
  return wifiDestIsLocal() && WiFi.status() == WL_CONNECTED &&
         (netConfig.payloadMode == PAYLOAD_RAW || netConfig.payloadMode == PAYLOAD_BOTH);
}

bool oscSendMidiEnabled() {
  return wifiDestIsLocal() && WiFi.status() == WL_CONNECTED &&
         (netConfig.payloadMode == PAYLOAD_MIDI || netConfig.payloadMode == PAYLOAD_BOTH);
}

void printWifiDestStatus() {
  if (!debugSerial) return;
  loadNetConfig();
  if (wifiDestIsLocal()) {
    Serial.print(F("Destino OSC local "));
    Serial.print(netConfig.oscIP);
    Serial.print(':');
    Serial.println(netConfig.oscPort);
    Serial.print(F("Payload OSC "));
    if (netConfig.payloadMode == PAYLOAD_MIDI) Serial.println(F("MIDI"));
    else if (netConfig.payloadMode == PAYLOAD_RAW) Serial.println(F("raw"));
    else Serial.println(F("raw+MIDI"));
  } else {
    Serial.println(F("Destino servidor online (MQTT)"));
  }
}

bool connectStaFromConfig() {
  loadNetConfig();
  WiFi.mode(WIFI_STA);

  if (debugSerial) Serial.println(F("Intentando conexion WiFi (hasta 15 s)..."));

  bool connected = false;
  if (netConfig.fromNvs && netConfig.ssid.length()) {
    if (debugSerial) {
      Serial.print(F("WiFi guardada: "));
      Serial.println(netConfig.ssid);
    }
    addNvsAp();
    connected = runMultiUntil(10000);
  }

  if (!connected) {
    if (debugSerial) Serial.println(F("WiFi: probando redes de secrets.h"));
    addSecretsAps();
    uint32_t budget = (netConfig.fromNvs && netConfig.ssid.length()) ? 7000 : 15000;
    connected = runMultiUntil(budget);
  }

  if (connected) currentSSID = WiFi.SSID();
  else currentSSID = "";
  if (debugSerial) Serial.println();
  return connected;
}

void sendConfigPage(const String &errorText) {
  loadNetConfig();
  String page;
  page.reserve(2200);
  page += F("<!DOCTYPE html><html lang=\"es\"><head><meta charset=\"utf-8\">");
  page += F("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
  page += F("<title>Biodata</title></head><body style=\"font-family:sans-serif;max-width:28rem;margin:1.5rem\">");
  page += F("<h1>Biodata</h1><p>Red WiFi y destino de los datos.</p>");
  if (errorText.length()) {
    page += F("<p>");
    page += htmlEscape(errorText);
    page += F("</p>");
  }
  page += F("<form method=\"POST\" action=\"http://biodata.sinfoniabiotica.xyz/save\">");
  page += F("<p><label>SSID<br><input name=\"ssid\" required maxlength=\"32\" value=\"");
  page += htmlEscape(netConfig.ssid.length() ? netConfig.ssid : String(WIFI_SSID));
  page += F("\"></label></p>");
  page += F("<p><label>Contrasena<br><input name=\"pass\" type=\"password\" maxlength=\"64\" placeholder=\"vacio = no cambiar\"></label></p>");
  page += F("<fieldset><legend>Destino</legend>");
  page += F("<label><input type=\"radio\" name=\"dest\" value=\"online\"");
  if (!wifiDestIsLocal()) page += F(" checked");
  page += F("> Servidor online (MQTT)</label><br>");
  page += F("<label><input type=\"radio\" name=\"dest\" value=\"local\"");
  if (wifiDestIsLocal()) page += F(" checked");
  page += F("> IP local (OSC)</label></fieldset>");
  page += F("<p><label>IP OSC<br><input name=\"oscip\" value=\"");
  page += htmlEscape(netConfig.oscIP);
  page += F("\"></label></p>");
  page += F("<p><label>Puerto OSC<br><input name=\"oscport\" type=\"number\" min=\"1\" max=\"65535\" value=\"");
  page += String(netConfig.oscPort);
  page += F("\"></label></p>");
  page += F("<p><label>Datos por OSC<br><select name=\"payload\">");
  page += F("<option value=\"midi\"");
  if (netConfig.payloadMode == PAYLOAD_MIDI) page += F(" selected");
  page += F(">Solo MIDI</option>");
  page += F("<option value=\"raw\"");
  if (netConfig.payloadMode == PAYLOAD_RAW) page += F(" selected");
  page += F(">Solo raw</option>");
  page += F("<option value=\"both\"");
  if (netConfig.payloadMode == PAYLOAD_BOTH) page += F(" selected");
  page += F(">Raw y MIDI</option></select></label></p>");
  page += F("<p><button type=\"submit\">Guardar y reiniciar</button></p></form></body></html>");
  configServer.send(200, "text/html", page);
}

void handleConfigRoot() {
  sendConfigPage("");
}

void handleConfigSave() {
  String ssid = configServer.arg("ssid");
  ssid.trim();
  if (!ssid.length() || ssid.length() > 32) {
    sendConfigPage("SSID no valido");
    return;
  }

  String pass = configServer.arg("pass");
  if (pass.length() > 64) pass = pass.substring(0, 64);
  if (!pass.length()) {
    if (netConfig.fromNvs && ssid == netConfig.ssid) pass = netConfig.pass;
    else if (!netConfig.fromNvs && ssid == String(WIFI_SSID)) pass = String(WIFI_PASSWORD);
    else {
      sendConfigPage("Falta la contrasena");
      return;
    }
  }

  String dest = configServer.arg("dest");
  uint8_t destMode = (dest == "local") ? DEST_LOCAL : DEST_ONLINE;

  String ip = configServer.arg("oscip");
  ip.trim();
  IPAddress parsed;
  if (!parsed.fromString(ip)) {
    sendConfigPage("IP OSC no valida");
    return;
  }

  int port = configServer.arg("oscport").toInt();
  if (port < 1 || port > 65535) {
    sendConfigPage("Puerto OSC no valido");
    return;
  }

  String pay = configServer.arg("payload");
  uint8_t payload = PAYLOAD_BOTH;
  if (pay == "midi") payload = PAYLOAD_MIDI;
  else if (pay == "raw") payload = PAYLOAD_RAW;

  netConfig.ssid = ssid;
  netConfig.pass = pass;
  netConfig.destMode = destMode;
  netConfig.oscIP = ip;
  netConfig.oscPort = (uint16_t)port;
  netConfig.payloadMode = payload;
  netConfig.fromNvs = true;
  saveNetConfig();

  // Salir del portal ya en WiFi ON
  wifiMIDI = 1;
  EEPROM.write(2, 1);
  EEPROM.commit();

  configServer.send(200, "text/html",
                    F("<!DOCTYPE html><html lang=\"es\"><body><h1>Guardado</h1><p>WiFi ON. Reiniciando...</p></body></html>"));
  delay(400);
  ESP.restart();
}

bool isPortalConfigHost() {
  String host = configServer.hostHeader();
  host.toLowerCase();
  int colon = host.indexOf(':');
  if (colon > 0) host = host.substring(0, colon);
  return host == PORTAL_HOST || host == "192.168.4.1" || host == PORTAL_IP.toString();
}

void redirectToPortalHost() {
  configServer.sendHeader(F("Location"), F("http://biodata.sinfoniabiotica.xyz/"), true);
  configServer.send(302, F("text/plain"), F(""));
}

void handlePortalRequest() {
  if (isPortalConfigHost()) {
    handleConfigRoot();
    return;
  }
  // Windows y otros abren msftconnecttest.com, etc. → forzar la URL del portal
  redirectToPortalHost();
}

void startConfigPortal() {
  EEPROM.commit();  // el portal puede reiniciar antes de que el menú haga commit
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char apName[16];
  snprintf(apName, sizeof(apName), "Biodata-%02X%02X", mac[4], mac[5]);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(PORTAL_IP, PORTAL_IP, PORTAL_MASK);
  WiFi.softAP(apName);

  if (debugSerial) {
    Serial.println(F("Portal de configuracion"));
    Serial.print(F("SSID "));
    Serial.println(apName);
    Serial.println(F("Abre http://biodata.sinfoniabiotica.xyz/"));
    Serial.print(F("(o http://"));
    Serial.print(WiFi.softAPIP());
    Serial.println(F(")"));
  }

  configDns.start(53, "*", WiFi.softAPIP());
  configServer.on("/", HTTP_GET, handlePortalRequest);
  configServer.on("/save", HTTP_POST, handleConfigSave);
  configServer.onNotFound([]() {
    if (configServer.method() == HTTP_GET) handlePortalRequest();
    else if (isPortalConfigHost() && configServer.uri() == "/save") handleConfigSave();
    else redirectToPortalHost();
  });
  configServer.begin();

  unsigned long start = millis();
  unsigned long ledAt = 0;
  bool ledOn = false;
  while (millis() - start < PORTAL_TIMEOUT_MS) {
    configDns.processNextRequest();
    configServer.handleClient();
    if (millis() - ledAt > 400) {
      ledAt = millis();
      ledOn = !ledOn;
      blinkRed(ledOn);
    }
    delay(2);
  }

  configServer.stop();
  configDns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  if (debugSerial) Serial.println(F("Portal cerrado por tiempo. WiFi sin conexion."));
}

// Menú LED azul. Si se guarda, el portal reinicia. Si se agota el tiempo, vuelve a la STA.
void openWifiConfigPortal() {
  if (audioMode) {
    if (debugSerial) Serial.println(F("Config WiFi solo en modo MIDI"));
    return;
  }
  if (debugSerial) Serial.println(F("Abriendo configuracion WiFi"));
  startConfigPortal();

  if (!wifiMIDI) {
    WiFi.disconnect(true);
    delay(1);
    WiFi.mode(WIFI_OFF);
    delay(1);
    return;
  }

  connectStaFromConfig();
  if (WiFi.status() != WL_CONNECTED) {
    ledFaders[0].Set(ledFaders[0].maxBright, 0);
    return;
  }

  currentSSID = WiFi.SSID();
  ledFaders[0].Set(0, 0);
  printWifiDestStatus();
  if (wifiDestIsLocal()) {
    oscStop();
    oscEnsureStarted();
    bufferEnabled = false;
  } else if (!bufferEnabled) {
    setupMQTT();
    bufferEnabled = true;
  }
}
