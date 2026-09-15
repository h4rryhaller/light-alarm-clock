#ifndef DISPLAY_H
#define DISPLAY_H

void setupDisplay();

// Muestra la hora actual y arma el apagado automático.
void showTime();

// Muestra un mensaje puntual y arma el apagado automático.
void showMessage(const char *msg);

// Debe llamarse en cada loop(); apaga el backlight/pantalla tras
// settings.presence.displayTimeoutMs sin actividad.
void displayTick();

#endif
