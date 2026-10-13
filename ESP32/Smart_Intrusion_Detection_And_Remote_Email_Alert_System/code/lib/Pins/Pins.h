#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

namespace Pins
{

    // Usable GPIOs for ESP32-WROOM-32
    extern const uint8_t GPIO0;
    extern const uint8_t GPIO1;
    extern const uint8_t GPIO2;
    extern const uint8_t GPIO3;
    extern const uint8_t GPIO4;
    extern const uint8_t GPIO5;
    extern const uint8_t GPIO12;
    extern const uint8_t GPIO13;
    extern const uint8_t GPIO14;
    extern const uint8_t GPIO15;
    extern const uint8_t GPIO16;
    extern const uint8_t GPIO17;
    extern const uint8_t GPIO18;
    extern const uint8_t GPIO19;
    extern const uint8_t GPIO21;
    extern const uint8_t GPIO22;
    extern const uint8_t GPIO23;
    extern const uint8_t GPIO25;
    extern const uint8_t GPIO26;
    extern const uint8_t GPIO27;
    extern const uint8_t GPIO32;
    extern const uint8_t GPIO33;
    extern const uint8_t GPIO34; // input only
    extern const uint8_t GPIO35; // input only
    extern const uint8_t GPIO36; // input only
    extern const uint8_t GPIO39; // input only

    void begin();
    int readPin(uint8_t gpio);
    void writePin(uint8_t gpio, bool value);

}

#endif
