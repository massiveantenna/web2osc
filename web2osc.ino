#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <ArduinoJson.h>

// Local config (not committed to git): Wi-Fi, rtacBase/rtacUser/rtacPass, oscIP/oscPort
#include "config.h"

// Tags to poll from the RTAC (Table 1 of the Implementation Manual).
// Each is queried at:  <rtacBase>/api/v1/logic-engine/symbols/Tags.<tag>
// and forwarded out as OSC:  /rtac/<tag>  (float, from the JSON "instMag" key)
const char* tags[] = {
  "Power_Consumption_Grid",
  "Power_Consumption_Solar",
  "Power_Ratio",
  "Energy_Solar_Today",
  "Energy_Grid_Today",
  "Power_Percent_Solar",
  "GHG_Prevented",
  "Vehicle_Equivalency",
  "Trees_Equivalency",
};
const size_t tagCount = sizeof(tags) / sizeof(tags[0]);

// Polling interval (milliseconds) — one full sweep of all tags per interval
const unsigned long pollInterval = 2000;
unsigned long lastPoll = 0;

WiFiUDP udp;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP: ");
  Serial.println(WiFi.localIP());

  udp.begin(9000); // local port for UDP
}

// Query one tag, parse instMag, forward as OSC. Returns true on success.
bool pollTag(const char* tag) {
  String url = String(rtacBase) + "/api/v1/logic-engine/symbols/Tags." + tag;

  HTTPClient http;

  // Pick a transport based on the URL scheme. The real RTAC uses https with a
  // device certificate; setInsecure() skips validation for the POC. Lock this
  // down with the device's real cert/fingerprint before production.
  if (url.startsWith("https")) {
    WiFiClientSecure client;
    client.setInsecure();
    if (!http.begin(client, url)) return false;
  } else {
    if (!http.begin(url)) return false;
  }

  http.setAuthorization(rtacUser, rtacPass); // HTTP Basic Auth (api / limbic)
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("  %s -> HTTP error %d\n", tag, httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("  %s -> JSON parse error: %s\n", tag, err.c_str());
    return false;
  }

  // Per the manual, the current value lives under "instMag".
  if (doc["instMag"].isNull()) {
    Serial.printf("  %s -> no 'instMag' in response\n", tag);
    return false;
  }
  float value = doc["instMag"].as<float>();

  // Forward as OSC: /rtac/<tag>  <float>
  String addr = String("/rtac/") + tag;
  OSCMessage msg(addr.c_str());
  msg.add(value);
  udp.beginPacket(oscIP, oscPort);
  msg.send(udp);
  udp.endPacket();
  msg.empty();

  Serial.printf("  %s = %.3f  -> OSC %s\n", tag, value, addr.c_str());
  return true;
}

void loop() {
  unsigned long now = millis();
  if (now - lastPoll < pollInterval) {
    return;
  }
  lastPoll = now;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected, reconnecting...");
    WiFi.reconnect();
    return;
  }

  Serial.println("Polling RTAC:");
  size_t ok = 0;
  for (size_t i = 0; i < tagCount; i++) {
    if (pollTag(tags[i])) ok++;
  }
  Serial.printf("Sweep done: %u/%u tags OK\n", (unsigned)ok, (unsigned)tagCount);
}
