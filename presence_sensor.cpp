#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "presence_sensor.h"

static unsigned long lastPollMs = 0;
static unsigned long lastGestureMs = 0;
static bool pendingGesture = false;

static const unsigned long GESTURE_DEBOUNCE_MS = 1000;

void setupPresenceSensor() {
  pinMode(PIN_HCSR04_TRIG, OUTPUT);
  pinMode(PIN_HCSR04_ECHO, INPUT);
  digitalWrite(PIN_HCSR04_TRIG, HIGH); // reposo en HIGH
}

static long measureDistanceCm() {
  digitalWrite(PIN_HCSR04_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_HCSR04_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_HCSR04_TRIG, LOW);

  unsigned long durationUs = pulseIn(PIN_HCSR04_ECHO, HIGH, 30000UL);
  if (durationUs == 0) return -1; // sin eco (fuera de rango o timeout)

  return static_cast<long>(durationUs / 58); // us -> cm
}

void presenceSensorTick() {
  unsigned long now = millis();
  if (now - lastPollMs < settings.presence.pollIntervalMs) return;
  lastPollMs = now;

  long distanceCm = measureDistanceCm();
  if (distanceCm < 0) return;

  if (distanceCm <= settings.presence.thresholdCm &&
      now - lastGestureMs >= GESTURE_DEBOUNCE_MS) {
    pendingGesture = true;
    lastGestureMs = now;
  }
}

bool gestureDetected() {
  if (!pendingGesture) return false;
  pendingGesture = false;
  return true;
}
