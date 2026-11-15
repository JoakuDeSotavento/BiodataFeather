// OSC UDP hacia una IP local. Solo con destino local y WiFi asociado.
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

static WiFiUDP oscUdp;
static bool oscUdpReady = false;

void oscStop() {
  if (!oscUdpReady) return;
  oscUdp.stop();
  oscUdpReady = false;
}

void oscEnsureStarted() {
  if (oscUdpReady) return;
  if (!wifiDestIsLocal() || WiFi.status() != WL_CONNECTED) return;
  if (!oscUdp.begin(0)) {
    if (debugSerial) Serial.println(F("OSC UDP no arranco"));
    return;
  }
  oscUdpReady = true;
  if (debugSerial) {
    Serial.print(F("OSC listo hacia "));
    Serial.print(netConfig.oscIP);
    Serial.print(':');
    Serial.println(netConfig.oscPort);
  }
}

bool sendOscPacket(OSCMessage &msg) {
  oscEnsureStarted();
  if (!oscUdpReady) return false;
  if (!oscUdp.beginPacket(netConfig.oscIP.c_str(), netConfig.oscPort)) return false;
  msg.send(oscUdp);
  oscUdp.endPacket();
  msg.empty();
  return true;
}

void sendOSCRaw(int averg, float stdevi, int thresholdMapped, int deltaMapped, int changeMapped) {
  if (!oscSendRawEnabled()) return;
  OSCMessage msg("/rawSig");
  msg.add((int32_t)averg);
  msg.add(stdevi);
  msg.add((int32_t)thresholdMapped);
  msg.add((int32_t)deltaMapped);
  msg.add((int32_t)changeMapped);
  sendOscPacket(msg);
}

void sendOSCNoteOn(int note, int velocity, long durationMs, int noteChannel) {
  if (!oscSendMidiEnabled()) return;
  OSCMessage msg("/note");
  msg.add((int32_t)note);
  msg.add((int32_t)velocity);
  msg.add((int32_t)durationMs);
  msg.add((int32_t)noteChannel);
  sendOscPacket(msg);
}

void sendOSCNoteOff(int note, int noteChannel) {
  if (!oscSendMidiEnabled()) return;
  OSCMessage msg("/noteOff");
  msg.add((int32_t)note);
  msg.add((int32_t)noteChannel);
  sendOscPacket(msg);
}

void sendOSCCC(int cc, int value, int noteChannel) {
  if (!oscSendMidiEnabled()) return;
  OSCMessage msg("/cc");
  msg.add((int32_t)cc);
  msg.add((int32_t)value);
  msg.add((int32_t)noteChannel);
  sendOscPacket(msg);
}
