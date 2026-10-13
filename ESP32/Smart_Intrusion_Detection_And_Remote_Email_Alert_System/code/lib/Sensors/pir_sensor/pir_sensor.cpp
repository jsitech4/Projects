#include <Arduino.h>
#include "pir_sensor.h"

namespace pir_sensor
{
  static uint8_t pirPin = 255;
  static bool currentState = false;
  static bool previousState = false;
  static bool motionStarted = false;
  static bool motionStopped = false;

  void begin(uint8_t pin)
  {
    pirPin = pin;
    pinMode(pirPin, INPUT);
    currentState = digitalRead(pirPin);
    previousState = currentState;
    motionStarted = false;
    motionStopped = false;
  }

  void update()
  {
    previousState = currentState;
    currentState = digitalRead(pirPin);

    motionStarted = (!previousState && currentState);
    motionStopped = (previousState && !currentState);
  }

  bool isMotionDetected()
  {
    return currentState;
  }

  bool isMotionStarted()
  {
    return motionStarted;
  }

  bool isMotionStopped()
  {
    return motionStopped;
  }
}
