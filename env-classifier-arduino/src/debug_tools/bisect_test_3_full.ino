// Bisection Test 3: everything from test 2, plus classify() and the fan
// digitalWrite -- this is the full real sketch in all but name. If this
// garbles (it should, to match what's been observed), we then comment
// out classify() OR the fan code (not both) to find out which one is
// actually responsible.

#include <LiquidCrystal.h>
#include <DHT.h>
#include <string.h>
#include "tree.h"

const uint8_t THERMISTOR_PIN = A0;
const uint8_t PHOTORESISTOR_PIN = A1;
const uint8_t DHT_PIN = 8;
const uint8_t FAN_PIN = 9;

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
float lastValidHumidity = 50.0;

bool shouldFanRun(EnvClass state) {
  return state == BRIGHT_WARM || state == DARK_WARM;
}

void setup() {
  dht.begin();
  lcd.begin(16, 2);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  lcd.setCursor(0, 0);
  lcd.print("Bisect test 3");
}

void loop() {
  unsigned long now = millis();

  if (now - lastDhtReadMs >= DHT_READ_INTERVAL_MS) {
    lastDhtReadMs = now;
    float dhtTemp = dht.readTemperature();
    float dhtHumidity = dht.readHumidity();
    if (!isnan(dhtTemp)) lastValidTemp = dhtTemp;
    if (!isnan(dhtHumidity)) lastValidHumidity = dhtHumidity;
    return;
  }

  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = now;

  int thermistorRaw = analogRead(THERMISTOR_PIN);
  int photoRaw = analogRead(PHOTORESISTOR_PIN);

  // --- This block is new vs test 2: classify() + fan digitalWrite ---
  EnvClass state = classify((float)thermistorRaw, (float)photoRaw, lastValidTemp, lastValidHumidity);
  bool fanOn = shouldFanRun(state);
  digitalWrite(FAN_PIN, fanOn ? HIGH : LOW);
  // --- end new block ---

  lcd.setCursor(0, 1);
  char row2[17];
  snprintf(row2, sizeof(row2), "T%d P%d %dC", thermistorRaw, photoRaw, (int)lastValidTemp);
  lcd.print(row2);
  for (int i = strlen(row2); i < 16; i++) {
    lcd.print(' ');
  }
}
