#include <Arduino.h>
#include "time_manager.h"

static bool timeSynced = false;

void initTime(const char *tz, const char *ntpServer1, const char *ntpServer2) {
  configTime(tz, ntpServer1, ntpServer2);
  timeSynced = false;
}

bool isTimeSynced() {
  if (timeSynced) return true;
  time_t now = time(nullptr);
  // Antes de sincronizar, time() devuelve un valor cercano a época 1970.
  timeSynced = now > 8 * 3600 * 2;
  return timeSynced;
}

bool getLocalTime(struct tm &outTime) {
  if (!isTimeSynced()) return false;
  time_t now = time(nullptr);
  localtime_r(&now, &outTime);
  return true;
}

long secondsUntilAlarm(uint8_t hour, uint8_t minute) {
  struct tm nowTm;
  if (!getLocalTime(nowTm)) return -1;

  struct tm target = nowTm;
  target.tm_hour = hour;
  target.tm_min = minute;
  target.tm_sec = 0;

  time_t now = mktime(&nowTm);
  time_t targetTime = mktime(&target);
  if (targetTime <= now) {
    targetTime += 24 * 3600;
  }
  return static_cast<long>(difftime(targetTime, now));
}
