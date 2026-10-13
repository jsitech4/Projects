#pragma once
#include <Arduino.h>
namespace email_manager { void begin(); bool queue(uint32_t eventId, uint32_t cycle, bool test=false); String status(); uint8_t pending(); }
