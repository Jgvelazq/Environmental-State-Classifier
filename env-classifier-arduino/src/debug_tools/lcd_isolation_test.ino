// LCD Isolation Test
//
// Does nothing except initialize the LCD and print static text. No
// sensors, no classifier, no fan logic -- if this still garbles, the
// problem is purely hardware/wiring. If this displays cleanly, the bug
// is specific to something in env_classifier.ino's LCD-handling code,
// not the physical connections.
//
// Upload this by itself (temporarily, in its own sketch folder) to
// test in isolation.

#include <LiquidCrystal.h>

const uint8_t LCD_RS = 7;
const uint8_t LCD_EN = 6;
const uint8_t LCD_D4 = 5;
const uint8_t LCD_D5 = 4;
const uint8_t LCD_D6 = 3;
const uint8_t LCD_D7 = 2;
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("LCD test row 1");
  lcd.setCursor(0, 1);
  lcd.print("LCD test row 2");
}

void loop() {
  // Intentionally empty -- text should be written once and stay static.
  // If it garbles or changes on its own with nothing in loop(), that's
  // a strong signal of a hardware/noise issue, not a software bug.
}
