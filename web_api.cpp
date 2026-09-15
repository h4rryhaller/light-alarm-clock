#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include "config.h"
#include "settings.h"
#include "alarm_engine.h"
#include "web_api.h"

static AsyncWebServer server(HTTP_PORT);

static void sendJson(AsyncWebServerRequest *request, int code, JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  request->send(code, "application/json", body);
}

static void handleStatus(AsyncWebServerRequest *request) {
  JsonDocument doc;
  doc["state"] = static_cast<int>(getAlarmState());
  doc["alarmEnabled"] = settings.alarm.enabled;
  doc["alarmHour"] = settings.alarm.hour;
  doc["alarmMinute"] = settings.alarm.minute;
  sendJson(request, 200, doc);
}

static void handleGetSettings(AsyncWebServerRequest *request) {
  // TODO: volcado completo de `settings` a JSON (Fase 7).
  JsonDocument doc;
  doc["version"] = settings.version;
  sendJson(request, 200, doc);
}

static void handleAcknowledge(AsyncWebServerRequest *request) {
  acknowledgeAlarm();
  request->send(204);
}

static void handleSleepStart(AsyncWebServerRequest *request) {
  startSleep();
  request->send(204);
}

static void handleSleepStop(AsyncWebServerRequest *request) {
  stopSleep();
  request->send(204);
}

static void handleSettingsReset(AsyncWebServerRequest *request) {
  applyDefaults(settings);
  saveSettings(settings);
  request->send(204);
}

static void notFound(AsyncWebServerRequest *request) {
  request->send(404, "application/json", "{\"error\":\"not found\"}");
}

void setupWebApi() {
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings/reset", HTTP_POST, handleSettingsReset);
  server.on("/api/alarm/acknowledge", HTTP_POST, handleAcknowledge);
  server.on("/api/sleep/start", HTTP_POST, handleSleepStart);
  server.on("/api/sleep/stop", HTTP_POST, handleSleepStop);

  // TODO Fase 7: GET/PUT /api/time, /api/alarm, /api/sleep, /api/wake, /api/presence
  // con (de)serialización JSON completa contra `settings`.

  server.onNotFound(notFound);
  server.begin();
  Serial.println("Servidor web API iniciado");
}
