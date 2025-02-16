#include "buzzer.h"

unsigned long buzzerStart = 0;
int buzzerCount = 0;
bool buzzerActive = false;
bool buzzerState = false;
int buzzerDelay = 100;
signed int buzzerMaxCount = 3;

void InitBuzzer(int _delayMs, bool _infinite = false, int _count = 3) {
  if (buzzerActive)
    return;
  buzzerDelay = _delayMs;
  buzzerMaxCount = (_infinite ? -999 : _count);
  buzzerCount = 0;
  buzzerState = false;
  buzzerStart = millis();
  buzzerActive = true;
}

void TriggerBuzzer() {
  if (!buzzerActive)
    return;

  #if DEBUG_PROC
  Serial.println("\nDEBUG: Buzzer is triggered!");
  #endif

  if (millis() - buzzerStart >= buzzerDelay) {
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    buzzerStart = millis();

    if(buzzerMaxCount != -999) {
      if (!buzzerState)
        buzzerCount++;

      if(buzzerCount > 0 && buzzerMaxCount >= buzzerCount)
        buzzerActive = false;
    }
  }
}
