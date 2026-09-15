#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <time.h>

// Arranca SNTP con TZ POSIX (configTime). No bloquea: la sincronización
// llega de forma asíncrona: isTimeSynced() indica cuándo ya hay hora válida.
void initTime(const char *tz, const char *ntpServer1, const char *ntpServer2);
bool isTimeSynced();

// Rellena outTime con la hora local actual. Devuelve false si aún no hay
// sincronización (outTime queda sin definir en ese caso).
bool getLocalTime(struct tm &outTime);

// Segundos que faltan hasta la próxima ocurrencia de hour:minute (0 si ya
// ha pasado hoy, se calcula para mañana). Devuelve -1 si no hay hora sincronizada.
long secondsUntilAlarm(uint8_t hour, uint8_t minute);

#endif
