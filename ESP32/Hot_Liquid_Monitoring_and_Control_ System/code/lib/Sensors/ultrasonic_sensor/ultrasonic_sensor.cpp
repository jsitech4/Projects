#include <Arduino.h>
#include <math.h>

#include "ultrasonic_sensor.h"

namespace ultrasonic_sensor
{
  static uint8_t trig = 255;
  static uint8_t echo = 255;

  static volatile uint32_t echoRiseUs = 0;
  static volatile uint32_t echoFallUs = 0;

  static volatile bool echoComplete = false;
  static volatile bool echoWaiting = false;

  static float distanceCm = NAN;

  static bool initialized = false;

  static constexpr uint32_t READ_INTERVAL_MS = 500;
  static constexpr uint32_t TRIGGER_HIGH_US = 10;
  static constexpr uint32_t ECHO_TIMEOUT_US = 30000UL;

  static uint32_t lastTriggerMs = 0;

  static bool triggerActive = false;
  static uint32_t triggerStartedUs = 0;

  static void IRAM_ATTR echoISR()
  {
    bool level = digitalRead(echo);

    uint32_t now = micros();

    if (level)
    {
      echoRiseUs = now;
      echoWaiting = true;
    }
    else
    {
      if (echoWaiting)
      {
        echoFallUs = now;
        echoComplete = true;
        echoWaiting = false;
      }
    }
  }

  void begin(uint8_t trigPin,
             uint8_t echoPin)
  {
    trig = trigPin;
    echo = echoPin;

    pinMode(trig, OUTPUT);
    pinMode(echo, INPUT);

    digitalWrite(trig, LOW);

    distanceCm = NAN;

    echoRiseUs = 0;
    echoFallUs = 0;

    echoComplete = false;
    echoWaiting = false;

    triggerActive = false;
    triggerStartedUs = 0;

    lastTriggerMs = millis();

    initialized = true;

    attachInterrupt(
        digitalPinToInterrupt(echo),
        echoISR,
        CHANGE);
  }

  void update()
  {
    if (!initialized)
    {
      return;
    }

    uint32_t nowMs = millis();
    uint32_t nowUs = micros();

    // -----------------------------------------------------
    // Finish 10 us trigger pulse
    // -----------------------------------------------------

    if (triggerActive)
    {
      if ((uint32_t)(nowUs - triggerStartedUs) >=
          TRIGGER_HIGH_US)
      {
        digitalWrite(trig, LOW);

        triggerActive = false;
      }

      return;
    }

    // -----------------------------------------------------
    // Process completed echo
    // -----------------------------------------------------

    if (echoComplete)
    {
      noInterrupts();

      uint32_t rise = echoRiseUs;
      uint32_t fall = echoFallUs;

      echoComplete = false;

      interrupts();

      uint32_t duration =
          fall - rise;

      if (duration > 0 &&
          duration <= ECHO_TIMEOUT_US)
      {
        float measuredDistance =
            ((float)duration * 0.0343f) /
            2.0f;

        if (measuredDistance >= 1.0f &&
            measuredDistance <= 500.0f)
        {
          distanceCm =
              measuredDistance;
        }
      }

      return;
    }

    // -----------------------------------------------------
    // Echo timeout
    // -----------------------------------------------------

    if (echoWaiting)
    {
      noInterrupts();

      uint32_t rise = echoRiseUs;

      interrupts();

      if ((uint32_t)(nowUs - rise) >=
          ECHO_TIMEOUT_US)
      {
        noInterrupts();

        echoWaiting = false;
        echoComplete = false;

        interrupts();

        distanceCm = NAN;
      }

      return;
    }

    // -----------------------------------------------------
    // Start new measurement
    // -----------------------------------------------------

    if ((uint32_t)(nowMs - lastTriggerMs) <
        READ_INTERVAL_MS)
    {
      return;
    }

    lastTriggerMs = nowMs;

    digitalWrite(trig, HIGH);

    triggerStartedUs = nowUs;

    triggerActive = true;
  }

  float getDistanceCm()
  {
    return distanceCm;
  }

  bool isValid()
  {
    return isfinite(distanceCm) &&
           distanceCm > 0.0f;
  }

  bool isObjectDetected(
      float thresholdCm)
  {
    return isValid() &&
           distanceCm > 0.0f &&
           distanceCm <= thresholdCm;
  }
}
