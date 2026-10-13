#pragma once
#include <Arduino.h>
#include "Control/intrusion_controller/intrusion_controller.h"
namespace event_log { void begin(); void record(const IntrusionStatus &state, bool closed); String json(); }
