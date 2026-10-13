#ifndef BATTERY_LEVEL_H
#define BATTERY_LEVEL_H

#include <Arduino.h>

namespace battery_level
{
  void begin(uint8_t pin, float scale = 1.61f);
  void update();
  void setScale(float scale);

  float getVoltage();
}

#endif
