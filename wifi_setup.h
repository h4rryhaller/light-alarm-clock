#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

// Bloqueante: lanza WiFiManager.autoConnect(apName). Si no hay red guardada,
// levanta un AP con ese nombre para configurarla desde el móvil. Debe
// completarse ANTES de arrancar el servidor async (ver web_api).
// Devuelve true si terminó conectado a una red.
bool setupWifi(const char *apName);

#endif
