#include <Arduino.h>
#include "config.h"
#include "led_control.h"

enum class LedEffect { NONE, FADE, FLASH };

static LedEffect activeEffect = LedEffect::NONE;

// Estado del fundido
static RGBColor fadeColor;
static uint8_t fadeTargetIntensity = 0;
static uint8_t currentIntensity = 0; // persiste entre llamadas: punto de partida de cada fundido
static uint32_t fadeCadenceMs = 0;
static uint8_t fadeStepPercent = 0;
static unsigned long fadeLastStepMs = 0;

// Estado del parpadeo
static RGBColor flashColorA;
static RGBColor flashColorB;
static uint32_t flashPeriodMs = 0;
static unsigned long flashLastToggleMs = 0;
static bool flashShowingA = true;

static void writeColor(const RGBColor &color, uint8_t intensityPercent) {
  float scale = intensityPercent / 100.0f;
  analogWrite(PIN_LED_R, static_cast<int>(color.r * scale));
  analogWrite(PIN_LED_G, static_cast<int>(color.g * scale));
  analogWrite(PIN_LED_B, static_cast<int>(color.b * scale));
}

void setupLed() {
  // Sin esto, analogWrite() usa el rango por defecto del core (0-1023) y los
  // valores de color 0-255 del resto del código solo llegan a ~25% de duty.
  analogWriteRange(255);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  stopLed();
}

void setLedIntensity(RGBColor color, uint8_t intensity) {
  currentIntensity = intensity;
  writeColor(color, currentIntensity);
}

void startFade(RGBColor color, uint8_t targetIntensity, uint32_t cadenceMs, uint8_t stepPercent) {
  activeEffect = LedEffect::FADE;
  fadeColor = color;
  fadeTargetIntensity = targetIntensity;
  fadeCadenceMs = cadenceMs;
  fadeStepPercent = stepPercent;
  fadeLastStepMs = millis();
  writeColor(fadeColor, currentIntensity);
}

void startFlash(RGBColor colorA, RGBColor colorB, uint32_t periodMs) {
  activeEffect = LedEffect::FLASH;
  flashColorA = colorA;
  flashColorB = colorB;
  flashPeriodMs = periodMs;
  flashLastToggleMs = millis();
  flashShowingA = true;
  writeColor(flashColorA, 100);
}

void stopLed() {
  activeEffect = LedEffect::NONE;
  currentIntensity = 0;
  analogWrite(PIN_LED_R, 0);
  analogWrite(PIN_LED_G, 0);
  analogWrite(PIN_LED_B, 0);
}

static void tickFade() {
  unsigned long now = millis();
  if (now - fadeLastStepMs < fadeCadenceMs) return;
  fadeLastStepMs = now;

  if (currentIntensity == fadeTargetIntensity) return;

  if (currentIntensity < fadeTargetIntensity) {
    uint16_t next = static_cast<uint16_t>(currentIntensity) + fadeStepPercent;
    currentIntensity = (next < fadeTargetIntensity) ? static_cast<uint8_t>(next) : fadeTargetIntensity;
  } else {
    uint8_t remaining = currentIntensity - fadeTargetIntensity;
    uint8_t step = (fadeStepPercent < remaining) ? fadeStepPercent : remaining;
    currentIntensity -= step;
  }
  writeColor(fadeColor, currentIntensity);
}

static void tickFlash() {
  unsigned long now = millis();
  if (now - flashLastToggleMs < flashPeriodMs) return;
  flashLastToggleMs = now;

  flashShowingA = !flashShowingA;
  writeColor(flashShowingA ? flashColorA : flashColorB, 100);
}

void ledTick() {
  switch (activeEffect) {
    case LedEffect::FADE:
      tickFade();
      break;
    case LedEffect::FLASH:
      tickFlash();
      break;
    case LedEffect::NONE:
    default:
      break;
  }
}
