#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "settings.h"

Settings settings;

void applyDefaults(Settings &s) {
  s.version = 1;

  strlcpy(s.time.tz, DEFAULT_TZ, sizeof(s.time.tz));
  strlcpy(s.time.ntpServer1, DEFAULT_NTP_SERVER_1, sizeof(s.time.ntpServer1));
  strlcpy(s.time.ntpServer2, DEFAULT_NTP_SERVER_2, sizeof(s.time.ntpServer2));

  s.alarm.enabled = true;
  s.alarm.hour = 7;
  s.alarm.minute = 30;

  s.sleep.color = {255, 140, 20};
  s.sleep.startIntensity = 100;
  s.sleep.cadenceMs = 2000;
  s.sleep.stepPercent = 2;
  s.sleep.durationMin = 30;

  s.wake.color = {255, 200, 100};
  s.wake.cadenceMs = 2000;
  s.wake.stepPercent = 2;
  s.wake.leadTimeMin = 60;
  s.wake.buzzLeadTimeMin = 20;
  s.wake.buzzIntervalMin = 5;
  s.wake.buzzBaseDurationMs = 150;
  s.wake.buzzDurationIncrementMs = 150;
  s.wake.buzzFrequencyHz = 2000;
  s.wake.alarmColors[0] = {255, 0, 0};
  s.wake.alarmColors[1] = {0, 0, 255};
  s.wake.alarmFlashCadenceMs = 300;
  s.wake.alarmBuzzFrequencyHz = 2500;
  s.wake.snoozeDurationMin = 5;
  s.wake.maxRingDurationMin = 30;

  s.presence.thresholdCm = 15;
  s.presence.pollIntervalMs = 150;
  s.presence.displayTimeoutMs = 5000;

  strlcpy(s.network.hostname, DEFAULT_HOSTNAME, sizeof(s.network.hostname));
  strlcpy(s.network.apName, DEFAULT_AP_NAME, sizeof(s.network.apName));
}

static void colorFromJson(JsonObjectConst obj, RGBColor &out) {
  out.r = obj["r"] | out.r;
  out.g = obj["g"] | out.g;
  out.b = obj["b"] | out.b;
}

static void colorToJson(JsonObject obj, const RGBColor &color) {
  obj["r"] = color.r;
  obj["g"] = color.g;
  obj["b"] = color.b;
}

static bool parseSettings(const JsonDocument &doc, Settings &s) {
  s.version = doc["version"] | s.version;

  JsonObjectConst time = doc["time"];
  strlcpy(s.time.tz, time["tz"] | s.time.tz, sizeof(s.time.tz));
  strlcpy(s.time.ntpServer1, time["ntpServer1"] | s.time.ntpServer1, sizeof(s.time.ntpServer1));
  strlcpy(s.time.ntpServer2, time["ntpServer2"] | s.time.ntpServer2, sizeof(s.time.ntpServer2));

  JsonObjectConst alarm = doc["alarm"];
  s.alarm.enabled = alarm["enabled"] | s.alarm.enabled;
  s.alarm.hour = alarm["hour"] | s.alarm.hour;
  s.alarm.minute = alarm["minute"] | s.alarm.minute;

  JsonObjectConst sleep = doc["sleep"];
  colorFromJson(sleep["color"], s.sleep.color);
  s.sleep.startIntensity = sleep["startIntensity"] | s.sleep.startIntensity;
  s.sleep.cadenceMs = sleep["cadenceMs"] | s.sleep.cadenceMs;
  s.sleep.stepPercent = sleep["stepPercent"] | s.sleep.stepPercent;
  s.sleep.durationMin = sleep["durationMin"] | s.sleep.durationMin;

  JsonObjectConst wake = doc["wake"];
  colorFromJson(wake["color"], s.wake.color);
  s.wake.cadenceMs = wake["cadenceMs"] | s.wake.cadenceMs;
  s.wake.stepPercent = wake["stepPercent"] | s.wake.stepPercent;
  s.wake.leadTimeMin = wake["leadTimeMin"] | s.wake.leadTimeMin;
  s.wake.buzzLeadTimeMin = wake["buzzLeadTimeMin"] | s.wake.buzzLeadTimeMin;
  s.wake.buzzIntervalMin = wake["buzzIntervalMin"] | s.wake.buzzIntervalMin;
  s.wake.buzzBaseDurationMs = wake["buzzBaseDurationMs"] | s.wake.buzzBaseDurationMs;
  s.wake.buzzDurationIncrementMs = wake["buzzDurationIncrementMs"] | s.wake.buzzDurationIncrementMs;
  s.wake.buzzFrequencyHz = wake["buzzFrequencyHz"] | s.wake.buzzFrequencyHz;
  JsonArrayConst alarmColors = wake["alarmColors"];
  if (alarmColors.size() >= 2) {
    colorFromJson(alarmColors[0], s.wake.alarmColors[0]);
    colorFromJson(alarmColors[1], s.wake.alarmColors[1]);
  }
  s.wake.alarmFlashCadenceMs = wake["alarmFlashCadenceMs"] | s.wake.alarmFlashCadenceMs;
  s.wake.alarmBuzzFrequencyHz = wake["alarmBuzzFrequencyHz"] | s.wake.alarmBuzzFrequencyHz;
  s.wake.snoozeDurationMin = wake["snoozeDurationMin"] | s.wake.snoozeDurationMin;
  s.wake.maxRingDurationMin = wake["maxRingDurationMin"] | s.wake.maxRingDurationMin;

  JsonObjectConst presence = doc["presence"];
  s.presence.thresholdCm = presence["thresholdCm"] | s.presence.thresholdCm;
  s.presence.pollIntervalMs = presence["pollIntervalMs"] | s.presence.pollIntervalMs;
  s.presence.displayTimeoutMs = presence["displayTimeoutMs"] | s.presence.displayTimeoutMs;

  JsonObjectConst network = doc["network"];
  strlcpy(s.network.hostname, network["hostname"] | s.network.hostname, sizeof(s.network.hostname));
  strlcpy(s.network.apName, network["apName"] | s.network.apName, sizeof(s.network.apName));

  return true;
}

bool loadSettings(Settings &s) {
  applyDefaults(s);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS: fallo al montar, uso defaults");
    return false;
  }

  File file = LittleFS.open(SETTINGS_FILE_PATH, "r");
  if (!file) {
    Serial.println("settings.json no existe, aplico defaults y guardo");
    saveSettings(s);
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.print("settings.json inválido (");
    Serial.print(err.c_str());
    Serial.println("), aplico defaults y guardo");
    saveSettings(s);
    return false;
  }

  parseSettings(doc, s);
  return true;
}

bool saveSettings(const Settings &s) {
  JsonDocument doc;

  doc["version"] = s.version;

  JsonObject time = doc["time"].to<JsonObject>();
  time["tz"] = s.time.tz;
  time["ntpServer1"] = s.time.ntpServer1;
  time["ntpServer2"] = s.time.ntpServer2;

  JsonObject alarm = doc["alarm"].to<JsonObject>();
  alarm["enabled"] = s.alarm.enabled;
  alarm["hour"] = s.alarm.hour;
  alarm["minute"] = s.alarm.minute;

  JsonObject sleep = doc["sleep"].to<JsonObject>();
  colorToJson(sleep["color"].to<JsonObject>(), s.sleep.color);
  sleep["startIntensity"] = s.sleep.startIntensity;
  sleep["cadenceMs"] = s.sleep.cadenceMs;
  sleep["stepPercent"] = s.sleep.stepPercent;
  sleep["durationMin"] = s.sleep.durationMin;

  JsonObject wake = doc["wake"].to<JsonObject>();
  colorToJson(wake["color"].to<JsonObject>(), s.wake.color);
  wake["cadenceMs"] = s.wake.cadenceMs;
  wake["stepPercent"] = s.wake.stepPercent;
  wake["leadTimeMin"] = s.wake.leadTimeMin;
  wake["buzzLeadTimeMin"] = s.wake.buzzLeadTimeMin;
  wake["buzzIntervalMin"] = s.wake.buzzIntervalMin;
  wake["buzzBaseDurationMs"] = s.wake.buzzBaseDurationMs;
  wake["buzzDurationIncrementMs"] = s.wake.buzzDurationIncrementMs;
  wake["buzzFrequencyHz"] = s.wake.buzzFrequencyHz;
  JsonArray alarmColors = wake["alarmColors"].to<JsonArray>();
  colorToJson(alarmColors.add<JsonObject>(), s.wake.alarmColors[0]);
  colorToJson(alarmColors.add<JsonObject>(), s.wake.alarmColors[1]);
  wake["alarmFlashCadenceMs"] = s.wake.alarmFlashCadenceMs;
  wake["alarmBuzzFrequencyHz"] = s.wake.alarmBuzzFrequencyHz;
  wake["snoozeDurationMin"] = s.wake.snoozeDurationMin;
  wake["maxRingDurationMin"] = s.wake.maxRingDurationMin;

  JsonObject presence = doc["presence"].to<JsonObject>();
  presence["thresholdCm"] = s.presence.thresholdCm;
  presence["pollIntervalMs"] = s.presence.pollIntervalMs;
  presence["displayTimeoutMs"] = s.presence.displayTimeoutMs;

  JsonObject network = doc["network"].to<JsonObject>();
  network["hostname"] = s.network.hostname;
  network["apName"] = s.network.apName;

  File file = LittleFS.open(SETTINGS_FILE_PATH, "w");
  if (!file) {
    Serial.println("No se pudo abrir settings.json para escritura");
    return false;
  }
  serializeJson(doc, file);
  file.close();
  return true;
}
