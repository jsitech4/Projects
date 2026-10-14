#pragma once
#include <Arduino.h>
#include <time.h>
namespace email_manager { void begin(); bool queue(uint32_t eventId, uint32_t cycle, bool test=false); bool queueIncident(uint32_t eventId, uint32_t cycle, time_t eventEpoch); String status(); uint8_t pending(); }
