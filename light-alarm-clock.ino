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
#include "crash_report.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("La caja — arrancando");
  reportSavedCrash();

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
