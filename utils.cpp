#include "utils.h"
unsigned long __startTime = 0;

void SafeDelay(unsigned long waitTime) {
  if (__startTime == 0)
    __startTime = millis();
  if (millis() - __startTime >= waitTime) {
    __startTime = 0;
    return;
  }
  yield();
}
