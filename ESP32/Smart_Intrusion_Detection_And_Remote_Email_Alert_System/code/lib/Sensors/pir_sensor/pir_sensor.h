#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#include <Arduino.h>

namespace pir_sensor
{
  void begin(uint8_t pin);
  void update();
  bool isMotionDetected();
  bool isMotionStarted();
  bool isMotionStopped();
}

#endif
