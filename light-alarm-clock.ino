#include "config.h"
#include "settings.h"
#include "display.h"
#include "wifi_setup.h"
#include "time_manager.h"
#include "web_api.h"
#include "led_control.h"
#include "buzzer_control.h"
#include "presence_sensor.h"
#include "alarm_engine.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("La caja — arrancando");

  loadSettings(settings);

  setupDisplay();
  showMessage("Iniciando...");

  setupWifi(settings.network.apName);

  initTime(settings.time.tz, settings.time.ntpServer1, settings.time.ntpServer2);

  setupWebApi();


  setupLed();
  setupBuzzer();
  setupPresenceSensor();

  setupAlarmEngine();

  Serial.println("Setup completo");

  /* --- TEST TEMPORAL: LED y buzzer por separado y simultáneo (verificado OK 2026-09-15) ---
  Serial.println("Test LED: rojo/verde/azul/blanco");
  setLedIntensity({255, 0, 0}, 100);
  delay(1000);
  setLedIntensity({0, 255, 0}, 100);
  delay(1000);
  setLedIntensity({0, 0, 255}, 100);
  delay(1000);
  setLedIntensity({255, 255, 255}, 100);
  delay(1000);
  setLedIntensity({0, 0, 0}, 0);

  Serial.println("Test buzzer: 3 beeps");
  for (int i = 0; i < 3; i++) {
    beep(1000, 200);
    unsigned long start = millis();
    while (millis() - start < 400) {
      buzzerTick();
    }
  }
  Serial.println("Test simultaneo: LED parpadeando + buzzer sonando 5s");
  startFlash({255, 0, 0}, {0, 0, 255}, 300);
  startContinuous(1000, 500, 200);
  unsigned long simStart = millis();
  while (millis() - simStart < 5000) {
    ledTick();
    buzzerTick();
  }
  stopLed();
  stopBuzzer();
  Serial.println("Test simultaneo completo");
  --- FIN TEST TEMPORAL --- */
}

void loop() {
  ledTick();
  buzzerTick();
  presenceSensorTick();
  displayTick();
  alarmEngineTick();

  if (gestureDetected()) {
    onGesture();
  }
}
