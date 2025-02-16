#include "utils.h"
void SafeDelay(unsigned long waitTime) {
  unsigned long startTime = millis();
  while (millis() - startTime < waitTime) {
    yield();
  }
}
