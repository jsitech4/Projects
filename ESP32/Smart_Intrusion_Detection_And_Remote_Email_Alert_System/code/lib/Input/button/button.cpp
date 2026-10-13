#include "button.h"
namespace button {
namespace { uint8_t pin = 255; bool stable = HIGH, previous = HIGH, triggered = false, held = false, longEvent = false; uint32_t changedAt = 0, pressedAt = 0, longPressMs = 5000; }
void begin(uint8_t value) { pin = value; pinMode(pin, INPUT); stable = previous = digitalRead(pin); }
void update() { bool reading = digitalRead(pin); uint32_t now = millis(); if (reading != previous) changedAt = now; if (reading != stable && now - changedAt >= 50) { stable = reading; if (stable == LOW) { pressedAt = now; held = false; } else if (!held) triggered = true; } if (stable == LOW && !held && now - pressedAt >= longPressMs) { held = true; longEvent = true; } previous = reading; }
bool isPressed() { return stable == LOW; }
bool isTrigger() { bool result = triggered; triggered = false; return result; }
bool isLongPress() { bool result = longEvent; longEvent = false; return result; }
void setLongPressMs(uint32_t value) { longPressMs = value < 1000 ? 1000 : value; }
}
