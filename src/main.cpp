#include <Arduino.h>
#include <WiFi.h>

#include "http_status_server.h"

static constexpr uint8_t kLedPin = 13;

#ifndef WIFI_SSID
#define WIFI_SSID "ENSandCo"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "elyseandspencer"
#endif

static void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);

  connectToWifi();
  startHttpStatusServer();
}

void loop() {
  delay(1000);
}
