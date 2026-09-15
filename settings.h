#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

struct RGBColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct TimeSettings {
  char tz[48];
  char ntpServer1[48];
  char ntpServer2[48];
};

struct AlarmSettings {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
};

struct SleepSettings {
  RGBColor color;
  uint8_t startIntensity;
  uint32_t cadenceMs;
  uint8_t stepPercent;
  uint16_t durationMin;
};

struct WakeSettings {
  RGBColor color;
  uint32_t cadenceMs;
  uint8_t stepPercent;
  uint16_t leadTimeMin;
  uint16_t buzzLeadTimeMin;
  uint16_t buzzIntervalMin;
  uint16_t buzzBaseDurationMs;
  uint16_t buzzDurationIncrementMs;
  uint16_t buzzFrequencyHz;
  RGBColor alarmColors[2];
  uint16_t alarmFlashCadenceMs;
  uint16_t alarmBuzzFrequencyHz;
  uint16_t snoozeDurationMin;
  uint16_t maxRingDurationMin;
};

struct PresenceSettings {
  uint16_t thresholdCm;
  uint16_t pollIntervalMs;
  uint32_t displayTimeoutMs;
};

struct NetworkSettings {
  char hostname[32];
  char apName[32];
};

struct Settings {
  uint8_t version;
  TimeSettings time;
  AlarmSettings alarm;
  SleepSettings sleep;
  WakeSettings wake;
  PresenceSettings presence;
  NetworkSettings network;
};

// Instancia global en memoria, compartida por el resto de módulos.
extern Settings settings;

void applyDefaults(Settings &s);
// Monta LittleFS (auto-formatea en el primer arranque) y carga SETTINGS_FILE_PATH.
// Si falta o no parsea, aplica defaults y guarda inmediatamente.
bool loadSettings(Settings &s);
bool saveSettings(const Settings &s);

#endif
