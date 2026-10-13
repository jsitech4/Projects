#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

namespace buzzer
{
  void begin(uint8_t pin);
  void update();

  void on();
  void off();
  void beep(uint16_t duration_ms);

  void startAlarm();
  void stopAlarm();

  bool isActive();
  bool isAlarmRunning();
}

#endif
