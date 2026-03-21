#ifndef UTILS_H
#define UTILS_H
#include "config.h"
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

void SafeDelay(unsigned long waitTime);
void scrollMessage(LiquidCrystal_I2C &lcd, int line, int row, String message, int scrollCount, int speed, bool clear = true, const char* finalmsg = "");
#endif
