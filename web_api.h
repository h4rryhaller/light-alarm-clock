#ifndef WEB_API_H
#define WEB_API_H

// Registra las rutas REST y arranca el servidor ESPAsyncWebServer.
// Requiere que el WiFi ya esté conectado (ver wifi_setup). Al ser asíncrono,
// no necesita ninguna llamada de tipo tick() en loop().
void setupWebApi();

#endif
