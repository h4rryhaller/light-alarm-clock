#include <Arduino.h>
#include "settings.h"
#include "time_manager.h"
#include "led_control.h"
#include "buzzer_control.h"
#include "display.h"
#include "alarm_engine.h"

// Patrón del buzzer en ALARM_ACTIVE: no forma parte del esquema JSON (solo
// define la frecuencia), así que se fija aquí como constante de implementación.
static const uint16_t ALARM_BUZZ_INTERVAL_MS = 800;
static const uint16_t ALARM_BUZZ_DURATION_MS = 400;

static AlarmState state = AlarmState::IDLE;

static unsigned long sleepStartMs = 0;
static unsigned long alarmStartMs = 0;      // para el freno de seguridad maxRingDurationMin
static unsigned long snoozeUntilMs = 0;
static unsigned long lastBuzzEscalationMs = 0;
static uint8_t buzzEscalationCount = 0;

// Instante (epoch) de la ocurrencia de alarma que armó WAKE_RAMP. Se fija una
// sola vez al entrar y no se recalcula en cada tick — secondsUntilAlarm()
// salta al día siguiente en cuanto la hora objetivo pasa, así que usarla
// dentro de WAKE_RAMP/WAKE_BUZZ_ESCALATION nunca deja ver secs<=0.
static time_t wakeTargetEpoch = 0;

static unsigned long minutesToMs(uint16_t minutes) {
  return static_cast<unsigned long>(minutes) * 60000UL;
}

static void enterIdle() {
  state = AlarmState::IDLE;
  stopLed();
  stopBuzzer();
}

static void enterWakeRamp() {
  state = AlarmState::WAKE_RAMP;
  buzzEscalationCount = 0;
  lastBuzzEscalationMs = millis();

  struct tm nowTm;
  getLocalTime(nowTm); // checkAutoWakeTrigger ya comprobó que hay hora sincronizada
  struct tm target = nowTm;
  target.tm_hour = settings.alarm.hour;
  target.tm_min = settings.alarm.minute;
  target.tm_sec = 0;
  wakeTargetEpoch = mktime(&target);
  if (wakeTargetEpoch < mktime(&nowTm)) wakeTargetEpoch += 24 * 3600;

  setLedIntensity(settings.wake.color, 0);
  startFade(settings.wake.color, 100, settings.wake.cadenceMs, settings.wake.stepPercent);
}

static void enterBuzzEscalation() {
  state = AlarmState::WAKE_BUZZ_ESCALATION;
  buzzEscalationCount = 0;
  lastBuzzEscalationMs = millis();
  // El LED sigue el mismo fundido iniciado en WAKE_RAMP, sin interrupción.
}

static void tickBuzzEscalation() {
  unsigned long now = millis();
  if (now - lastBuzzEscalationMs < minutesToMs(settings.wake.buzzIntervalMin)) return;
  lastBuzzEscalationMs = now;

  uint16_t duration = settings.wake.buzzBaseDurationMs +
                       buzzEscalationCount * settings.wake.buzzDurationIncrementMs;
  beep(settings.wake.buzzFrequencyHz, duration);
  buzzEscalationCount++;
}

static void enterAlarmActive() {
  state = AlarmState::ALARM_ACTIVE;
  alarmStartMs = millis();
  startFlash(settings.wake.alarmColors[0], settings.wake.alarmColors[1], settings.wake.alarmFlashCadenceMs);
  startContinuous(settings.wake.alarmBuzzFrequencyHz, ALARM_BUZZ_INTERVAL_MS, ALARM_BUZZ_DURATION_MS);
}

static void checkAutoWakeTrigger() {
  if (!settings.alarm.enabled) return;
  long secs = secondsUntilAlarm(settings.alarm.hour, settings.alarm.minute);
  if (secs < 0) return; // hora aún no sincronizada
  if (secs <= static_cast<long>(settings.wake.leadTimeMin) * 60) {
    enterWakeRamp();
  }
}

void setupAlarmEngine() {
  enterIdle();
}

void startSleep() {
  if (state != AlarmState::IDLE) return;
  state = AlarmState::SLEEP_MODE;
  sleepStartMs = millis();
  setLedIntensity(settings.sleep.color, settings.sleep.startIntensity);
  startFade(settings.sleep.color, 0, settings.sleep.cadenceMs, settings.sleep.stepPercent);
}

void stopSleep() {
  if (state != AlarmState::SLEEP_MODE) return;
  enterIdle();
}

void onGesture() {
  showTime();

  switch (state) {
    case AlarmState::SLEEP_MODE:
      // README: el gesto reinicia la función dormir.
      sleepStartMs = millis();
      setLedIntensity(settings.sleep.color, settings.sleep.startIntensity);
      startFade(settings.sleep.color, 0, settings.sleep.cadenceMs, settings.sleep.stepPercent);
      break;
    case AlarmState::ALARM_ACTIVE:
      // README: el gesto silencia el buzzer, nunca desactiva la alarma. El LED sigue.
      stopBuzzer();
      state = AlarmState::ALARM_SNOOZED;
      snoozeUntilMs = millis() + minutesToMs(settings.wake.snoozeDurationMin);
      break;
    case AlarmState::ALARM_SNOOZED:
      // Gesto repetido: reinicia el temporizador de snooze.
      snoozeUntilMs = millis() + minutesToMs(settings.wake.snoozeDurationMin);
      break;
    default:
      break;
  }
}

void acknowledgeAlarm() {
  if (state != AlarmState::ALARM_ACTIVE && state != AlarmState::ALARM_SNOOZED) return;
  enterIdle();
}

AlarmState getAlarmState() {
  return state;
}

void alarmEngineTick() {
  switch (state) {
    case AlarmState::IDLE:
      checkAutoWakeTrigger();
      break;

    case AlarmState::SLEEP_MODE:
      checkAutoWakeTrigger(); // la alarma debe poder interrumpir el modo dormir
      if (state != AlarmState::SLEEP_MODE) break; // se interrumpió: ya está en WAKE_RAMP
      if (millis() - sleepStartMs >= minutesToMs(settings.sleep.durationMin)) {
        enterIdle();
      }
      break;

    case AlarmState::WAKE_RAMP: {
      double secs = difftime(wakeTargetEpoch, time(nullptr));
      if (secs <= static_cast<double>(settings.wake.buzzLeadTimeMin) * 60) {
        enterBuzzEscalation();
      }
      break;
    }

    case AlarmState::WAKE_BUZZ_ESCALATION: {
      double secs = difftime(wakeTargetEpoch, time(nullptr));
      if (secs <= 0) {
        enterAlarmActive();
      } else {
        tickBuzzEscalation();
      }
      break;
    }

    case AlarmState::ALARM_ACTIVE:
      if (millis() - alarmStartMs >= minutesToMs(settings.wake.maxRingDurationMin)) {
        enterIdle(); // freno de seguridad: nadie ha reconocido la alarma
      }
      break;

    case AlarmState::ALARM_SNOOZED:
      if (millis() >= snoozeUntilMs) {
        enterAlarmActive();
      }
      break;
  }
}
