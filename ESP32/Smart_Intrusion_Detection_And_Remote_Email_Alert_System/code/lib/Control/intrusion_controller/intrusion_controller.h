#pragma once
#include <Arduino.h>

struct IntrusionStatus {
  bool rawPir = false;
  bool confirmedMotion = false;
  bool alarmActive = false;
  bool warmingUp = true;
  uint32_t eventId = 0;
  uint32_t alarmCycle = 0;
  uint32_t remainingAlarmMs = 0;
  uint32_t totalEvents = 0;
  uint32_t lastEventMs = 0;
};

namespace intrusion_controller {
  typedef void (*NotificationCallback)(uint32_t eventId, uint32_t cycle, bool test);
  typedef void (*EventCallback)(const IntrusionStatus &status, bool closed);
  void begin(NotificationCallback notification, EventCallback event);
  void update();
  void simulateMotion();
  void stopAudibleOutput();
  void testBuzzer(uint16_t ms = 500);
  const IntrusionStatus &status();
}
