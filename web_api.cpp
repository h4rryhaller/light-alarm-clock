#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include "config.h"
#include "settings.h"
#include "alarm_engine.h"
#include "time_manager.h"
#include "web_api.h"

static AsyncWebServer server(HTTP_PORT);

static void sendJson(AsyncWebServerRequest *request, int code, JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  request->send(code, "application/json", body);
}

// Si el cliente declara Content-Type: application/x-www-form-urlencoded (p.ej.
// curl -d por defecto, sin -H "Content-Type: application/json") en un PUT con
// cuerpo JSON, ESPAsyncWebSrv nunca llama a nuestro manejador de body: mete el
// cuerpo entero en un parámetro POST llamado "body" y no responde nada (el
// cliente ve "empty reply", reproducible al 100%). Este helper recupera ese
// parámetro desde el manejador de "request" (que sí se llama siempre, pase lo
// que pase con el body) para que la API funcione sin depender de que el
// cliente mande el Content-Type correcto. En el caso normal (Content-Type
// correcto) el body ya se procesó en el manejador de body y no hay parámetro
// "body" que recuperar, así que esto no hace nada.
static void handleMisroutedBody(AsyncWebServerRequest *request, void (*apply)(AsyncWebServerRequest *, const String &)) {
  AsyncWebParameter *p = request->getParam("body", true);
  if (p) apply(request, p->value());
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
  JsonDocument doc;
  settingsToJson(settings, doc);
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

static void handlePresenceGet(AsyncWebServerRequest *request) {
  JsonDocument doc;
  doc["thresholdCm"] = settings.presence.thresholdCm;
  doc["pollIntervalMs"] = settings.presence.pollIntervalMs;
  doc["displayTimeoutMs"] = settings.presence.displayTimeoutMs;
  sendJson(request, 200, doc);
}

static void applyPresenceJson(AsyncWebServerRequest *request, const String &body) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  settings.presence.thresholdCm = doc["thresholdCm"] | settings.presence.thresholdCm;
  settings.presence.pollIntervalMs = doc["pollIntervalMs"] | settings.presence.pollIntervalMs;
  settings.presence.displayTimeoutMs = doc["displayTimeoutMs"] | settings.presence.displayTimeoutMs;
  saveSettings(settings);

  handlePresenceGet(request);
}

// Acumula el body en trozos (puede llegar en varios paquetes TCP) y solo
// parsea/aplica cuando index+len alcanza total.
static String presencePutBody;

static void handlePresencePutBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) presencePutBody = "";
  presencePutBody.concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) return;
  applyPresenceJson(request, presencePutBody);
}

static void handlePresencePutRequest(AsyncWebServerRequest *request) {
  handleMisroutedBody(request, applyPresenceJson);
}

static void handleTimeGet(AsyncWebServerRequest *request) {
  JsonDocument doc;
  struct tm t;
  bool synced = getLocalTime(t);
  doc["synced"] = synced;
  if (synced) {
    doc["year"] = t.tm_year + 1900;
    doc["month"] = t.tm_mon + 1;
    doc["day"] = t.tm_mday;
    doc["hour"] = t.tm_hour;
    doc["minute"] = t.tm_min;
    doc["second"] = t.tm_sec;
  }
  sendJson(request, 200, doc);
}

// Fallback manual sin NTP: si aún no hay hora sincronizada, parte de
// 2026-01-01 00:00:00 y aplica encima los campos que llegan en el body.
static void applyTimeJson(AsyncWebServerRequest *request, const String &body) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  struct tm t;
  if (!getLocalTime(t)) {
    memset(&t, 0, sizeof(t));
    t.tm_year = 126; // 2026
    t.tm_mday = 1;
  }
  t.tm_year = (doc["year"] | (t.tm_year + 1900)) - 1900;
  t.tm_mon = (doc["month"] | (t.tm_mon + 1)) - 1;
  t.tm_mday = doc["day"] | t.tm_mday;
  t.tm_hour = doc["hour"] | t.tm_hour;
  t.tm_min = doc["minute"] | t.tm_min;
  t.tm_sec = doc["second"] | t.tm_sec;
  setManualTime(t);

  handleTimeGet(request);
}

static String timePutBody;

static void handleTimePutBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) timePutBody = "";
  timePutBody.concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) return;
  applyTimeJson(request, timePutBody);
}

static void handleTimePutRequest(AsyncWebServerRequest *request) {
  handleMisroutedBody(request, applyTimeJson);
}

static void handleAlarmGet(AsyncWebServerRequest *request) {
  JsonDocument doc;
  doc["enabled"] = settings.alarm.enabled;
  doc["hour"] = settings.alarm.hour;
  doc["minute"] = settings.alarm.minute;
  sendJson(request, 200, doc);
}

static void applyAlarmJson(AsyncWebServerRequest *request, const String &body) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  settings.alarm.enabled = doc["enabled"] | settings.alarm.enabled;
  settings.alarm.hour = doc["hour"] | settings.alarm.hour;
  settings.alarm.minute = doc["minute"] | settings.alarm.minute;
  saveSettings(settings);

  handleAlarmGet(request);
}

static String alarmPutBody;

static void handleAlarmPutBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) alarmPutBody = "";
  alarmPutBody.concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) return;
  applyAlarmJson(request, alarmPutBody);
}

static void handleAlarmPutRequest(AsyncWebServerRequest *request) {
  handleMisroutedBody(request, applyAlarmJson);
}

static void handleSleepGet(AsyncWebServerRequest *request) {
  JsonDocument doc;
  colorToJson(doc["color"].to<JsonObject>(), settings.sleep.color);
  doc["startIntensity"] = settings.sleep.startIntensity;
  doc["cadenceMs"] = settings.sleep.cadenceMs;
  doc["stepPercent"] = settings.sleep.stepPercent;
  doc["durationMin"] = settings.sleep.durationMin;
  sendJson(request, 200, doc);
}

static void applySleepJson(AsyncWebServerRequest *request, const String &body) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  // colorFromJson ya es segura sin este campo (cada r/g/b conserva su valor
  // si no viene en el JSON, vía el operador |), igual que el resto de campos
  // de esta misma función.
  colorFromJson(doc["color"], settings.sleep.color);
  settings.sleep.startIntensity = doc["startIntensity"] | settings.sleep.startIntensity;
  settings.sleep.cadenceMs = doc["cadenceMs"] | settings.sleep.cadenceMs;
  settings.sleep.stepPercent = doc["stepPercent"] | settings.sleep.stepPercent;
  settings.sleep.durationMin = doc["durationMin"] | settings.sleep.durationMin;
  saveSettings(settings);

  handleSleepGet(request);
}

static String sleepPutBody;

static void handleSleepPutBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) sleepPutBody = "";
  sleepPutBody.concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) return;
  applySleepJson(request, sleepPutBody);
}

static void handleSleepPutRequest(AsyncWebServerRequest *request) {
  handleMisroutedBody(request, applySleepJson);
}

static void handleWakeGet(AsyncWebServerRequest *request) {
  JsonDocument doc;
  colorToJson(doc["color"].to<JsonObject>(), settings.wake.color);
  doc["cadenceMs"] = settings.wake.cadenceMs;
  doc["stepPercent"] = settings.wake.stepPercent;
  doc["leadTimeMin"] = settings.wake.leadTimeMin;
  doc["buzzLeadTimeMin"] = settings.wake.buzzLeadTimeMin;
  doc["buzzIntervalMin"] = settings.wake.buzzIntervalMin;
  doc["buzzBaseDurationMs"] = settings.wake.buzzBaseDurationMs;
  doc["buzzDurationIncrementMs"] = settings.wake.buzzDurationIncrementMs;
  doc["buzzFrequencyHz"] = settings.wake.buzzFrequencyHz;
  JsonArray alarmColors = doc["alarmColors"].to<JsonArray>();
  colorToJson(alarmColors.add<JsonObject>(), settings.wake.alarmColors[0]);
  colorToJson(alarmColors.add<JsonObject>(), settings.wake.alarmColors[1]);
  doc["alarmFlashCadenceMs"] = settings.wake.alarmFlashCadenceMs;
  doc["alarmBuzzFrequencyHz"] = settings.wake.alarmBuzzFrequencyHz;
  doc["snoozeDurationMin"] = settings.wake.snoozeDurationMin;
  doc["maxRingDurationMin"] = settings.wake.maxRingDurationMin;
  sendJson(request, 200, doc);
}

static void applyWakeJson(AsyncWebServerRequest *request, const String &body) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  // Mismo motivo que en applySleepJson: colorFromJson ya es segura sin este campo.
  colorFromJson(doc["color"], settings.wake.color);
  settings.wake.cadenceMs = doc["cadenceMs"] | settings.wake.cadenceMs;
  settings.wake.stepPercent = doc["stepPercent"] | settings.wake.stepPercent;
  settings.wake.leadTimeMin = doc["leadTimeMin"] | settings.wake.leadTimeMin;
  settings.wake.buzzLeadTimeMin = doc["buzzLeadTimeMin"] | settings.wake.buzzLeadTimeMin;
  settings.wake.buzzIntervalMin = doc["buzzIntervalMin"] | settings.wake.buzzIntervalMin;
  settings.wake.buzzBaseDurationMs = doc["buzzBaseDurationMs"] | settings.wake.buzzBaseDurationMs;
  settings.wake.buzzDurationIncrementMs = doc["buzzDurationIncrementMs"] | settings.wake.buzzDurationIncrementMs;
  settings.wake.buzzFrequencyHz = doc["buzzFrequencyHz"] | settings.wake.buzzFrequencyHz;
  JsonArrayConst alarmColors = doc["alarmColors"];
  if (alarmColors.size() >= 2) {
    colorFromJson(alarmColors[0], settings.wake.alarmColors[0]);
    colorFromJson(alarmColors[1], settings.wake.alarmColors[1]);
  }
  settings.wake.alarmFlashCadenceMs = doc["alarmFlashCadenceMs"] | settings.wake.alarmFlashCadenceMs;
  settings.wake.alarmBuzzFrequencyHz = doc["alarmBuzzFrequencyHz"] | settings.wake.alarmBuzzFrequencyHz;
  settings.wake.snoozeDurationMin = doc["snoozeDurationMin"] | settings.wake.snoozeDurationMin;
  settings.wake.maxRingDurationMin = doc["maxRingDurationMin"] | settings.wake.maxRingDurationMin;
  saveSettings(settings);

  handleWakeGet(request);
}

static String wakePutBody;

static void handleWakePutBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) wakePutBody = "";
  wakePutBody.concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) return;
  applyWakeJson(request, wakePutBody);
}

static void handleWakePutRequest(AsyncWebServerRequest *request) {
  handleMisroutedBody(request, applyWakeJson);
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

  server.on("/api/presence", HTTP_GET, handlePresenceGet);
  server.on("/api/presence", HTTP_PUT, handlePresencePutRequest, NULL, handlePresencePutBody);

  server.on("/api/time", HTTP_GET, handleTimeGet);
  server.on("/api/time", HTTP_PUT, handleTimePutRequest, NULL, handleTimePutBody);

  server.on("/api/alarm", HTTP_GET, handleAlarmGet);
  server.on("/api/alarm", HTTP_PUT, handleAlarmPutRequest, NULL, handleAlarmPutBody);

  server.on("/api/sleep", HTTP_GET, handleSleepGet);
  server.on("/api/sleep", HTTP_PUT, handleSleepPutRequest, NULL, handleSleepPutBody);

  server.on("/api/wake", HTTP_GET, handleWakeGet);
  server.on("/api/wake", HTTP_PUT, handleWakePutRequest, NULL, handleWakePutBody);

  server.onNotFound(notFound);
  server.begin();
  Serial.println("Servidor web API iniciado");
}
