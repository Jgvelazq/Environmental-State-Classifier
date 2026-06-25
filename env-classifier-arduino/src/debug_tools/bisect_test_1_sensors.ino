// Bisection Test 1: LCD + analog sensor reads only
//
// Adds thermistor/photoresistor analogRead() on top of the known-good
// LCD isolation test -- still no DHT11, no classify(), no fan. If this
// garbles, analogRead() itself (or its timing relative to LCD writes)
// is implicated. If this stays clean, the problem is further down the
// list (DHT11, classify(), or the fan pin).

#include <LiquidCrystal.h>
#include <string.h>

const uint8_t THERMISTOR_PIN = A0;
const uint8_t PHOTORESISTOR_PIN = A1;

const uint8_t LCD_RS = 7;
const uint8_t LCD_EN = 6;
const uint8_t LCD_D4 = 5;
const uint8_t LCD_D5 = 4;
const uint8_t LCD_D6 = 3;
const uint8_t LCD_D7 = 2;
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

const unsigned long SAMPLE_INTERVAL_MS = 1000;
unsigned long lastSampleMs = 0;

void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Bisect test 1");
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = now;

  int thermistorRaw = analogRead(THERMISTOR_PIN);
  int photoRaw = analogRead(PHOTORESISTOR_PIN);

  lcd.setCursor(0, 1);
  char row2[17];
  snprintf(row2, sizeof(row2), "T%d P%d", thermistorRaw, photoRaw);
  lcd.print(row2);
  for (int i = strlen(row2); i < 16; i++) {
    lcd.print(' ');
  }
}
