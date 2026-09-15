#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "settings.h"

void setupLed();

// Fija intensidad (0-100) y color de forma inmediata, sin transición.
// Útil para fijar el punto de partida de un fundido (p.ej. dormir empieza en startIntensity).
void setLedIntensity(RGBColor color, uint8_t intensity);

// Fundido no bloqueante: parte de la intensidad actual y camina hacia
// `targetIntensity` (0-100) en pasos de `stepPercent` cada `cadenceMs`, con el
// color dado. Sirve tanto para subir (despertar) como para bajar (dormir),
// según se llame antes o no a setLedIntensity().
void startFade(RGBColor color, uint8_t targetIntensity, uint32_t cadenceMs, uint8_t stepPercent);

// Parpadeo no bloqueante alternando entre dos colores cada periodMs, a máxima intensidad.
void startFlash(RGBColor colorA, RGBColor colorB, uint32_t periodMs);

void stopLed();

// Debe llamarse en cada loop(); actualiza el PWM según el efecto activo.
void ledTick();

#endif
