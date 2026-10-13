#include "Pins.h"

namespace Pins
{

  const uint8_t GPIO0 = 0;
  const uint8_t GPIO1 = 1;
  const uint8_t GPIO2 = 2;
  const uint8_t GPIO3 = 3;
  const uint8_t GPIO4 = 4;
  const uint8_t GPIO5 = 5;
  const uint8_t GPIO12 = 12;
  const uint8_t GPIO13 = 13;
  const uint8_t GPIO14 = 14;
  const uint8_t GPIO15 = 15;
  const uint8_t GPIO16 = 16;
  const uint8_t GPIO17 = 17;
  const uint8_t GPIO18 = 18;
  const uint8_t GPIO19 = 19;
  const uint8_t GPIO21 = 21;
  const uint8_t GPIO22 = 22;
  const uint8_t GPIO23 = 23;
  const uint8_t GPIO25 = 25;
  const uint8_t GPIO26 = 26;
  const uint8_t GPIO27 = 27;
  const uint8_t GPIO32 = 32;
  const uint8_t GPIO33 = 33;
  const uint8_t GPIO34 = 34; // input only
  const uint8_t GPIO35 = 35; // input only
  const uint8_t GPIO36 = 36; // input only
  const uint8_t GPIO39 = 39; // input only

  void begin()
  {
    // Initialize pinModes here pins
  }

  int readPin(uint8_t gpio)
  {
    return digitalRead(gpio);
  }

  void writePin(uint8_t gpio, bool value)
  {
    digitalWrite(gpio, value ? HIGH : LOW);
  }

}
