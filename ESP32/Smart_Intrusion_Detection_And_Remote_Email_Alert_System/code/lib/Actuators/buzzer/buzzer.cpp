#include <Arduino.h>
#include "Pins.h"
#include "buzzer.h"

namespace buzzer
{
  static uint8_t BUZZER_PIN = 255;

  static bool active = false;
  static bool beeping = false;
  static bool alarmRunning = false;

  static unsigned long beepStart = 0;
  static unsigned long beepDuration = 0;

  static unsigned long alarmStartTime = 0;
  static unsigned long alarmStepStart = 0;

  static const unsigned long alarmDuration = 20000;

  static uint8_t alarmStep = 0;

  void begin(uint8_t pin)
  {
    BUZZER_PIN = pin;
    pinMode(BUZZER_PIN, OUTPUT);
    Pins::writePin(BUZZER_PIN, LOW);
  }

  void on()
  {
    active = true;
    Pins::writePin(BUZZER_PIN, HIGH);
  }

  void off()
  {
    active = false;
    Pins::writePin(BUZZER_PIN, LOW);
  }

  void beep(uint16_t duration_ms)
  {
    alarmRunning = false;

    if (beeping)
      return;

    beepDuration = duration_ms;
    beepStart = millis();
    beeping = true;
    on();
  }

  void startAlarm()
  {
    beeping = false;

    if (!alarmRunning)
    {
      alarmRunning = true;
      alarmStep = 0;
      alarmStepStart = millis();
    }

    alarmStartTime = millis();
  }

  void stopAlarm()
  {
    alarmRunning = false;
    beeping = false;
    alarmStep = 0;
    off();
  }

  bool isActive()
  {
    return active;
  }

  bool isAlarmRunning()
  {
    return alarmRunning;
  }

  void update()
  {
    unsigned long now = millis();

    if (beeping)
    {
      if (now - beepStart >= beepDuration)
      {
        off();
        beeping = false;
      }
    }

    if (!alarmRunning)
      return;

    if (now - alarmStartTime >= alarmDuration)
    {
      stopAlarm();
      return;
    }

    switch (alarmStep)
    {
    case 0:
      on();
      alarmStepStart = now;
      alarmStep = 1;
      break;

    case 1:
      if (now - alarmStepStart >= 150)
      {
        off();
        alarmStepStart = now;
        alarmStep = 2;
      }
      break;

    case 2:
      if (now - alarmStepStart >= 100)
      {
        on();
        alarmStepStart = now;
        alarmStep = 3;
      }
      break;

    case 3:
      if (now - alarmStepStart >= 150)
      {
        off();
        alarmStepStart = now;
        alarmStep = 4;
      }
      break;

    case 4:
      if (now - alarmStepStart >= 400)
      {
        alarmStep = 0;
      }
      break;
    }
  }
}
