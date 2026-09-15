#include "config.h"
#include "buzzer_control.h"

enum class BuzzerMode { IDLE, BEEP, CONTINUOUS };

static BuzzerMode mode = BuzzerMode::IDLE;

// Beep puntual
static unsigned long beepEndMs = 0;

// Patrón continuo
static uint16_t continuousFreq = 0;
static uint16_t continuousIntervalMs = 0;
static uint16_t continuousDurationMs = 0;
static unsigned long continuousLastStartMs = 0;
static bool continuousSounding = false;

void setupBuzzer() {
  pinMode(PIN_BUZZER, OUTPUT);
  noTone(PIN_BUZZER);
}

void beep(uint16_t freq, uint16_t durationMs) {
  mode = BuzzerMode::BEEP;
  tone(PIN_BUZZER, freq, durationMs);
  beepEndMs = millis() + durationMs;
}

void startContinuous(uint16_t freq, uint16_t intervalMs, uint16_t durationMs) {
  mode = BuzzerMode::CONTINUOUS;
  continuousFreq = freq;
  continuousIntervalMs = intervalMs;
  continuousDurationMs = durationMs;
  continuousLastStartMs = 0;
  continuousSounding = false;
}

void stopBuzzer() {
  mode = BuzzerMode::IDLE;
  noTone(PIN_BUZZER);
  continuousSounding = false;
}

static void tickContinuous() {
  unsigned long now = millis();
  if (continuousSounding) {
    if (now - continuousLastStartMs >= continuousDurationMs) {
      noTone(PIN_BUZZER);
      continuousSounding = false;
    }
    return;
  }
  if (now - continuousLastStartMs >= continuousIntervalMs) {
    tone(PIN_BUZZER, continuousFreq, continuousDurationMs);
    continuousLastStartMs = now;
    continuousSounding = true;
  }
}

void buzzerTick() {
  switch (mode) {
    case BuzzerMode::BEEP:
      if (millis() >= beepEndMs) mode = BuzzerMode::IDLE;
      break;
    case BuzzerMode::CONTINUOUS:
      tickContinuous();
      break;
    case BuzzerMode::IDLE:
    default:
      break;
  }
}
