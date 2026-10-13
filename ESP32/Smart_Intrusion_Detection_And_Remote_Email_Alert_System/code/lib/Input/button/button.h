#pragma once
#include <Arduino.h>
namespace button {
  void begin(uint8_t pin);
  void update();
  bool isPressed();
  bool isTrigger();       // released before the long-press limit: simulated PIR event
  bool isLongPress();     // emitted once after the configured hold period
  void setLongPressMs(uint32_t value);
}
