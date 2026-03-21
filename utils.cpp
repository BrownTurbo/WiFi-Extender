#include "utils.h"

void SafeDelay(unsigned long waitTime) {
  unsigned long __startTime = millis();
  while (millis() - __startTime < waitTime) {
    yield();
  }
}

void scrollMessage(LiquidCrystal_I2C &lcd, int line, int row, String message, int scrollCount, int speed, bool clear, const char* finalmsg) {
    int messageLen = message.length();
    
    for (int iteration = 0; iteration < scrollCount; iteration++) {
        lcd.clear();
        lcd.setCursor(line, row);
        lcd.print(message);

        for (int i = 0; i < (messageLen + 16); i++) {
            lcd.scrollDisplayLeft();
            delay(speed);
        }
        lcd.clear();
    }

    if (clear)
        lcd.clear();
    else {
        lcd.home(); 
        lcd.setCursor(line, row);
        lcd.print(finalmsg);
    }
}