#pragma once

#include <Arduino.h>

namespace ultrasonic_sensor
{
  void begin(uint8_t trigPin,
             uint8_t echoPin);

  void update();

  float getDistanceCm();

  bool isValid();

  bool isObjectDetected(float thresholdCm);
}
