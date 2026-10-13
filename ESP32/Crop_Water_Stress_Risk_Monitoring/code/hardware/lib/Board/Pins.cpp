#include "Pins.h"
bool cws::pins::mappingValid() {
    constexpr int pins[] = {32, 34, 35, 39, 21, 22, 25, 26, 27, 2, 4};
    for (auto p : pins)
        if (p == 14 || p == 12 || p == 33 || p == 16)
            return false;
    return true;
}
