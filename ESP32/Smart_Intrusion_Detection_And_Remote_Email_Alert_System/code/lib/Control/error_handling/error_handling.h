#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <Arduino.h>

namespace error_handling
{

  void begin();
  void enableWatchdog(uint8_t timeoutSeconds = 10);
  void update();
  bool watchdogEnabled();

  void setCodeError(bool state);
  bool hasError();

}

#endif
