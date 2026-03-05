#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <ArduinoJson.h>

// Local config (not committed to git)
#include "config.h"

// Web API endpoint (JSONPlaceholder for testing)
const char* apiURL = "https://jsonplaceholder.typicode.com/todos/1";

// Polling interval (milliseconds)
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

  HTTPClient http;
  http.begin(apiURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.print("Response: ");
    Serial.println(payload);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
      Serial.print("JSON parse error: ");
      Serial.println(err.c_str());
    } else {
      int32_t id = doc["id"];
      const char* title = doc["title"];
      bool completed = doc["completed"];

      // Send each field as an OSC message
      OSCMessage msgId("/api/id");
      msgId.add(id);
      udp.beginPacket(oscIP, oscPort);
      msgId.send(udp);
      udp.endPacket();
      msgId.empty();

      OSCMessage msgTitle("/api/title");
      msgTitle.add(title);
      udp.beginPacket(oscIP, oscPort);
      msgTitle.send(udp);
      udp.endPacket();
      msgTitle.empty();

      OSCMessage msgDone("/api/completed");
      msgDone.add((int32_t)(completed ? 1 : 0));
      udp.beginPacket(oscIP, oscPort);
      msgDone.send(udp);
      udp.endPacket();
      msgDone.empty();

      Serial.println("OSC sent");
    }
  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
  }

  http.end();
}
