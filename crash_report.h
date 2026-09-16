#ifndef CRASH_REPORT_H
#define CRASH_REPORT_H

// Diagnóstico temporal para los resets con rst cause:2 de los que no se
// consigue volcado de excepción por Serial (se pierde en la carrera contra
// el propio reinicio, típico en ESP8266). custom_crash_callback() guarda los
// registros del crash en memoria RTC, que sobrevive a cualquier reset salvo
// un corte de alimentación; reportSavedCrash() los vuelca por Serial con
// calma en el siguiente arranque y los borra. Llamar justo después de
// Serial.begin() en setup().
void reportSavedCrash();

#endif
