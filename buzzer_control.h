#ifndef BUZZER_CONTROL_H
#define BUZZER_CONTROL_H

#include <Arduino.h>

void setupBuzzer();

// Toque puntual no bloqueante: suena freq durante durationMs y luego calla.
void beep(uint16_t freq, uint16_t durationMs);

// Patrón insistente: suena durationMs cada intervalMs, de forma continuada
// hasta stopBuzzer(). Pensado para ALARM_ACTIVE.
void startContinuous(uint16_t freq, uint16_t intervalMs, uint16_t durationMs);

void stopBuzzer();

// Debe llamarse en cada loop(); gestiona los temporizadores de beep()/startContinuous().
void buzzerTick();

#endif
