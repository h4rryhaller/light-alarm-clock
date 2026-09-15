#ifndef PRESENCE_SENSOR_H
#define PRESENCE_SENSOR_H

void setupPresenceSensor();

// Debe llamarse en cada loop(); sondea el HC-SR04 cada
// settings.presence.pollIntervalMs (no en cada iteración).
void presenceSensorTick();

// true una única vez por gesto detectado (con debounce); se consume al leerlo.
bool gestureDetected();

#endif
