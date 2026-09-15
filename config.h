#ifndef CONFIG_H
#define CONFIG_H

// ---- Pines (NodeMCU ESP8266) ----
// I2C (LCD) usa los pines por defecto de Wire: SCL=D1/GPIO5, SDA=D2/GPIO4
#define PIN_LED_R      D7  // GPIO13, analogWrite
#define PIN_LED_G      D6  // GPIO12, analogWrite
#define PIN_LED_B      D5  // GPIO14, analogWrite
#define PIN_HCSR04_TRIG D3 // GPIO0  — reposo en HIGH (pull-up interno)
#define PIN_HCSR04_ECHO D8 // GPIO15 — reposo en LOW; requiere divisor de tensión 5V->3.3V
#define PIN_BUZZER     3   // RX / GPIO3 (tone()/noTone())

// ---- LCD I2C ----
#define LCD_COLUMNS 16
#define LCD_ROWS    2
#define LCD_I2C_ADDRESS 0x27 // dirección habitual de estos backpacks I2C

// ---- Servidor web / persistencia ----
#define HTTP_PORT 80
#define SETTINGS_FILE_PATH "/settings.json"

// ---- Valores por defecto ----
#define DEFAULT_TZ "CET-1CEST,M3.5.0,M10.5.0/3"
#define DEFAULT_NTP_SERVER_1 "pool.ntp.org"
#define DEFAULT_NTP_SERVER_2 "time.nist.gov"
#define DEFAULT_HOSTNAME "la-caja"
#define DEFAULT_AP_NAME "LaCaja-Setup"

#endif