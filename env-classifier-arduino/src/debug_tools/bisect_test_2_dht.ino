// Bisection Test 2: LCD + analog sensors + DHT11
//
// Adds the DHT11 on top of bisect_test_1 -- still no classify(), no
// fan. If this garbles (and test 1 was clean), the DHT11 is implicated
// after all, just not exactly the mechanism first suspected. If this
// stays clean too, the problem is in classify() or the fan pin/code,
// not the sensors themselves.

#include <LiquidCrystal.h>
#include <DHT.h>
#include <string.h>

const uint8_t THERMISTOR_PIN = A0;
const uint8_t PHOTORESISTOR_PIN = A1;
const uint8_t DHT_PIN = 8;

const uint8_t LCD_RS = 7;
const uint8_t LCD_EN = 6;
const uint8_t LCD_D4 = 5;
const uint8_t LCD_D5 = 4;
const uint8_t LCD_D6 = 3;
const uint8_t LCD_D7 = 2;
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

const unsigned long SAMPLE_INTERVAL_MS = 1000;
unsigned long lastSampleMs = 0;
const unsigned long DHT_READ_INTERVAL_MS = 2500;
unsigned long lastDhtReadMs = 0;
float lastValidTemp = 25.0;

void setup() {
  dht.begin();
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Bisect test 2");
}

void loop() {
  unsigned long now = millis();

  if (now - lastDhtReadMs >= DHT_READ_INTERVAL_MS) {
    lastDhtReadMs = now;
    float dhtTemp = dht.readTemperature();
    if (!isnan(dhtTemp)) lastValidTemp = dhtTemp;
    return;
  }

  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = now;

  int thermistorRaw = analogRead(THERMISTOR_PIN);
  int photoRaw = analogRead(PHOTORESISTOR_PIN);

  lcd.setCursor(0, 1);
  char row2[17];
  snprintf(row2, sizeof(row2), "T%d P%d %dC", thermistorRaw, photoRaw, (int)lastValidTemp);
  lcd.print(row2);
  for (int i = strlen(row2); i < 16; i++) {
    lcd.print(' ');
  }
}
