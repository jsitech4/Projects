#include <Arduino.h>
#include "ultrasonic_sensor.h"

namespace ultrasonic_sensor
{
  static uint8_t trig = 255;
  static uint8_t echo = 255;

  static volatile uint32_t echoStartUs = 0;
  static volatile uint32_t echoDurationUs = 0;
  static volatile bool echoComplete = false;

  static float distanceCm = 0.0f;

  static const uint32_t READ_INTERVAL_MS = 500;
  static const uint32_t TRIGGER_HIGH_US = 10;
  static const uint32_t ECHO_TIMEOUT_US = 30000UL;

  static uint32_t lastReadMs = 0;
  static uint32_t triggerStartUs = 0;

  static bool waitingForEcho = false;

  static void IRAM_ATTR echoISR()
  {
    const uint32_t nowUs = micros();

    if (digitalRead(echo) == HIGH)
    {
      echoStartUs = nowUs;
    }
    else
    {
      if (echoStartUs != 0)
      {
        echoDurationUs = nowUs - echoStartUs;
        echoComplete = true;
      }
    }
  }

  void begin(uint8_t trigPin, uint8_t echoPin)
  {
    trig = trigPin;
    echo = echoPin;

    pinMode(trig, OUTPUT);
    pinMode(echo, INPUT);

    digitalWrite(trig, LOW);

    echoStartUs = 0;
    echoDurationUs = 0;
    echoComplete = false;

    distanceCm = 0.0f;

    lastReadMs = millis();
    triggerStartUs = 0;

    waitingForEcho = false;

    attachInterrupt(
        digitalPinToInterrupt(echo),
        echoISR,
        CHANGE);
  }
  void update()
  {
    const uint32_t nowMs = millis();
    const uint32_t nowUs = micros();

    if (echoComplete)
    {
      noInterrupts();

      const uint32_t duration = echoDurationUs;

      echoComplete = false;
      echoStartUs = 0;

      interrupts();

      if (duration > 0 && duration <= ECHO_TIMEOUT_US)
      {
        const float measuredDistance =
            (duration * 0.0343f) / 2.0f;

        if (measuredDistance >= 2.0f &&
            measuredDistance <= 500.0f)
        {
          distanceCm = measuredDistance;
        }
      }

      waitingForEcho = false;
    }

    if (waitingForEcho)
    {
      if ((uint32_t)(nowUs - triggerStartUs) >= ECHO_TIMEOUT_US)
      {
        waitingForEcho = false;

        noInterrupts();
        echoStartUs = 0;
        echoDurationUs = 0;
        echoComplete = false;
        interrupts();
      }

      return;
    }

    if ((uint32_t)(nowMs - lastReadMs) < READ_INTERVAL_MS)
    {
      return;
    }

    lastReadMs = nowMs;

    digitalWrite(trig, LOW);

    delayMicroseconds(2);

    digitalWrite(trig, HIGH);

    delayMicroseconds(TRIGGER_HIGH_US);

    digitalWrite(trig, LOW);

    triggerStartUs = micros();

    waitingForEcho = true;
  }

  float getDistanceCm()
  {
    return distanceCm;
  }

  bool isObjectDetected(float thresholdCm)
  {
    return distanceCm > 0.0f &&
           distanceCm <= thresholdCm;
  }
}
