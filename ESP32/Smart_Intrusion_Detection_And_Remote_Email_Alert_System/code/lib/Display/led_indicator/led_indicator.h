#pragma once
#include <Arduino.h>

namespace led_indicator {
  // GPIO2 is the LED used by the original PCB firmware. It is active-HIGH.
  void begin(uint8_t pin, bool activeHigh = true);
  void set(bool enabled);
  bool isOn();
}
