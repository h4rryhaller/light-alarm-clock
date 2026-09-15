#ifndef ALARM_ENGINE_H
#define ALARM_ENGINE_H

enum class AlarmState {
  IDLE,
  SLEEP_MODE,
  WAKE_RAMP,
  WAKE_BUZZ_ESCALATION,
  ALARM_ACTIVE,
  ALARM_SNOOZED
};

void setupAlarmEngine();

// Debe llamarse en cada loop(); avanza la máquina de estados.
void alarmEngineTick();

// IDLE -> SLEEP_MODE. No-op si no está en IDLE.
void startSleep();

// SLEEP_MODE -> IDLE (cancelación manual). No-op en cualquier otro estado.
void stopSleep();

// Efecto de un gesto detectado por el sensor de presencia: siempre refresca
// la pantalla; además reinicia el fundido en SLEEP_MODE o pausa el buzzer en
// ALARM_ACTIVE/ALARM_SNOOZED (nunca desactiva la alarma).
void onGesture();

// Parada administrativa desde la API: ALARM_ACTIVE/ALARM_SNOOZED -> IDLE.
// No-op en cualquier otro estado.
void acknowledgeAlarm();

AlarmState getAlarmState();

#endif
