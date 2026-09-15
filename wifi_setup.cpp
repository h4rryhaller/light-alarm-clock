#include <Arduino.h>
#include <WiFiManager.h>
#include "wifi_setup.h"

bool setupWifi(const char *apName) {
  WiFiManager wifiManager;

  Serial.println("WiFi: intentando autoConnect...");
  bool connected = wifiManager.autoConnect(apName);

  if (connected) {
    Serial.print("WiFi conectado, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi: no se pudo conectar (timeout del portal de configuración)");
  }

  return connected;
}
