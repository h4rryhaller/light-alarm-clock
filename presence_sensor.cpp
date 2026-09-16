#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "led_control.h"
#include "presence_sensor.h"

static unsigned long lastPollMs = 0;
static unsigned long lastGestureMs = 0;
static bool pendingGesture = false;

// 1000ms hacía que una mano quieta delante del sensor repintara la pantalla
// (y, si el LED de dormir/despertar estaba activo, reiniciara su fundido)
// una vez por segundo sin necesidad — el README solo pide mostrar la hora al
// gesto, no en bucle. De paso, menos repintados por minuto = menos ocasiones
// de coincidir con el PWM del LED (sospecha detrás de los rst cause:2 vistos
// con gestos repetidos + LED activo).
static const unsigned long GESTURE_DEBOUNCE_MS = 3000;

// Diagnóstico temporal: sospecha de que el softPWM del LED (analogWrite)
// interfiere con la medición por pulseIn() y produce lecturas erráticas
// (falsos gestos y/o gestos reales no detectados). Se retira una vez
// confirmada o descartada la hipótesis con datos reales.
static const bool PRESENCE_DEBUG_LOG = true;
static const unsigned long LOG_INTERVAL_MS = 1000;

// El muestreo real sigue yendo a pollIntervalMs (no se toca la cadencia de
// detección); esto solo agrega varias muestras en una línea por segundo,
// quedándonos con la lectura más baja (la más "cercana"/sospechosa) del tramo.
static unsigned long lastLogMs = 0;
static long windowMinCm = -1;
static uint16_t windowSamples = 0;
static uint16_t windowTimeouts = 0;
static bool windowLedActive = false;
static bool windowGesture = false;

void setupPresenceSensor() {
  pinMode(PIN_HCSR04_TRIG, OUTPUT);
  pinMode(PIN_HCSR04_ECHO, INPUT);
  digitalWrite(PIN_HCSR04_TRIG, HIGH); // reposo en HIGH
}

static long measureDistanceCm(unsigned long &durationUsOut) {
  digitalWrite(PIN_HCSR04_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_HCSR04_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_HCSR04_TRIG, LOW);

  durationUsOut = pulseIn(PIN_HCSR04_ECHO, HIGH, 30000UL);
  if (durationUsOut == 0) return -1; // sin eco (fuera de rango o timeout)

  return static_cast<long>(durationUsOut / 58); // us -> cm
}

void presenceSensorTick() {
  unsigned long now = millis();

  if (now - lastPollMs >= settings.presence.pollIntervalMs) {
    lastPollMs = now;

    unsigned long durationUs = 0;
    long distanceCm = measureDistanceCm(durationUs);

    bool willFire = distanceCm >= 0 &&
                    distanceCm <= settings.presence.thresholdCm &&
                    now - lastGestureMs >= GESTURE_DEBOUNCE_MS;

    if (PRESENCE_DEBUG_LOG) {
      windowSamples++;
      if (distanceCm < 0) {
        windowTimeouts++;
      } else if (windowMinCm < 0 || distanceCm < windowMinCm) {
        windowMinCm = distanceCm;
      }
      if (isLedEffectActive()) windowLedActive = true;
      if (willFire) windowGesture = true;
    }

    if (distanceCm >= 0 && willFire) {
      pendingGesture = true;
      lastGestureMs = now;
    }
  }

  if (PRESENCE_DEBUG_LOG && now - lastLogMs >= LOG_INTERVAL_MS) {
    lastLogMs = now;
    // heap: para confirmar/descartar una fuga de memoria como causa de los
    // resets tras varios minutos de uptime (rst cause:2 visto en pruebas
    // largas). Si baja de forma sostenida hasta el crash, es fuga; si se
    // mantiene estable, hay que buscar la causa en otro sitio.
    Serial.printf("[presence] t=%lu n=%d min_cm=%ld timeouts=%d thr=%d led=%d gesture=%d heap=%u\n",
                  now, windowSamples, windowMinCm, windowTimeouts,
                  settings.presence.thresholdCm, windowLedActive ? 1 : 0,
                  windowGesture ? 1 : 0, ESP.getFreeHeap());
    windowSamples = 0;
    windowTimeouts = 0;
    windowMinCm = -1;
    windowLedActive = false;
    windowGesture = false;
  }
}

bool gestureDetected() {
  if (!pendingGesture) return false;
  pendingGesture = false;
  return true;
}
