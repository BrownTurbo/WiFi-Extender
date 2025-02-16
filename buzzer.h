#ifndef BUZZER_H
#define BUZZER_H
#include "config.h"
#include <Arduino.h>

extern unsigned long buzzerStart;
extern int buzzerCount;
extern bool buzzerActive;
extern bool buzzerState;
extern int buzzerDelay;
extern signed int buzzerMaxCount;

void InitBuzzer(int delayMs, bool _infinite, int count);
void TriggerBuzzer();
#endif
