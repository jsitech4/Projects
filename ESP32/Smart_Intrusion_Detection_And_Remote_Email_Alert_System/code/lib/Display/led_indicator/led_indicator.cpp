#include "led_indicator.h"

namespace led_indicator {
namespace { uint8_t ledPin = 255; bool highPolarity = true; bool on = false; }

void begin(uint8_t pin, bool activeHigh) {
  ledPin = pin;
  highPolarity = activeHigh;
  pinMode(ledPin, OUTPUT);
  set(false); // never energize the LED unexpectedly during boot
}

void set(bool enabled) {
  on = enabled;
  if (ledPin != 255) digitalWrite(ledPin, (enabled == highPolarity) ? HIGH : LOW);
}

bool isOn() { return on; }
}
