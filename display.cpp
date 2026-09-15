#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "settings.h"
#include "time_manager.h"
#include "display.h"

static LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
static unsigned long lastActivityMs = 0;
static bool backlightOn = false;

static void wake() {
  lcd.backlight();
  backlightOn = true;
  lastActivityMs = millis();
}

void setupDisplay() {
  lcd.init();
  lcd.noBacklight();
  backlightOn = false;
}

void showTime() {
  wake();
  lcd.clear();

  struct tm now;
  if (!getLocalTime(now)) {
    lcd.setCursor(0, 0);
    lcd.print("Sin hora");
    return;
  }

  char buf[LCD_COLUMNS + 1];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.tm_hour, now.tm_min, now.tm_sec);
  lcd.setCursor(0, 0);
  lcd.print(buf);
}

void showMessage(const char *msg) {
  wake();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
}

void displayTick() {
  if (!backlightOn) return;
  if (millis() - lastActivityMs >= settings.presence.displayTimeoutMs) {
    lcd.noBacklight();
    backlightOn = false;
  }
}
