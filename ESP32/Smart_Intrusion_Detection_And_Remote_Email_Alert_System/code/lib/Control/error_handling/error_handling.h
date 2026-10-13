#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <Arduino.h>

namespace error_handling
{

  void begin();
  void update();

  void setCodeError(bool state);
  bool hasError();

}

#endif
