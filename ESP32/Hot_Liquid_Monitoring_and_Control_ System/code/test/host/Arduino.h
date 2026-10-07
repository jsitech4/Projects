#pragma once

#include <cstdint>

// Only the Arduino interfaces used by the real modules under test.
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t HIGH = 1;
constexpr uint8_t LOW = 0;

unsigned long millis();
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
double ledcSetup(uint8_t channel, double frequency, uint8_t resolution);
void ledcAttachPin(uint8_t pin, uint8_t channel);
void ledcWrite(uint8_t channel, uint32_t duty);
