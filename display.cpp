#include "config.h"

// LCDBigNumbers (Library Manager) necesita saber el tipo de pantalla antes de
// incluirla: fija el tamaño (16x2) y trae su propia LiquidCrystal_I2C interna
// (misma API que la habitual: init/backlight/noBacklight/setCursor/print...).
// Si el hardware pasa a un LCD 20x4, cambiar esto a USE_SERIAL_2004_LCD además
// de LCD_ROWS/LCD_COLUMNS en config.h.
#define USE_SERIAL_1602_LCD
#include <LCDBigNumbers.hpp>

#include "settings.h"
#include "time_manager.h"
#include "display.h"

static LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
// Fuente 2 columnas x 2 filas por dígito (Alpenglow Industries), ya probada
// por terceros en hardware real — evita depender de una fuente diseñada a
// mano que no se puede verificar sin el LCD delante.
static LCDBigNumbers bigNumbers(&lcd, BIG_NUMBERS_FONT_2_COLUMN_2_ROWS_VARIANT_1);

static unsigned long lastActivityMs = 0;
static bool backlightOn = false;

// Columna de arranque para centrar "HH:MM" (2+1 + 2+1 + 1 + 2+1 + 2 ≈ 13
// columnas de 16). Es aproximado a propósito: la librería añade un hueco de
// separación tras cada dígito, así que el centrado exacto se ajusta a ojo.
static const uint8_t BIG_TIME_START_COL = 1;

static void wake() {
  lcd.backlight();
  backlightOn = true;
  lastActivityMs = millis();
}

// Diagnóstico temporal: para descartar que el problema esté en la escritura
// de CGRAM (memoria de caracteres personalizados) en vez de en la tabla de
// fuentes de LCDBigNumbers, se crea "a mano" una carita simple en el slot 0 y
// se muestra 3s al arrancar, sin pasar por la lógica de dígitos grandes. Si
// esto también sale mal/irreconocible, el problema es de más bajo nivel
// (I2C/CGRAM), no de la librería de números grandes. Quitar cuando ya no
// haga falta.
static const bool DISPLAY_SELFTEST = true;
static uint8_t TEST_SMILEY[8] = {0x00, 0x0A, 0x0A, 0x00, 0x11, 0x0E, 0x00, 0x00};

void setupDisplay() {
  lcd.init();
  lcd.noBacklight();
  backlightOn = false;

  bigNumbers.begin();

  if (DISPLAY_SELFTEST) {
    lcd.createChar(0, TEST_SMILEY);
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write((uint8_t)0);
    lcd.setCursor(0, 1);
    lcd.write((uint8_t)0);
    delay(3000);
    bigNumbers.begin(); // restaura el slot 0 con el patrón real de dígitos
    lcd.noBacklight();
  }
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

  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", now.tm_hour, now.tm_min);
  Serial.print(F("[display] showTime buf=\""));
  Serial.print(buf);
  Serial.println('"');

  bigNumbers.setBigNumberCursor(BIG_TIME_START_COL, 0);
  bigNumbers.print(buf);
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
