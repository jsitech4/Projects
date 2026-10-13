#include "intrusion_controller.h"
#include "Storage/config/config.h"
#include "Sensors/pir_sensor/pir_sensor.h"
#include "Actuators/buzzer/buzzer.h"

namespace
{
  IntrusionStatus state;
  uint32_t bootMs, rawChangedMs, alarmEndsMs;
  bool rawPrevious;
  intrusion_controller::NotificationCallback notifyCb;
  intrusion_controller::EventCallback eventCb;
  bool elapsed(uint32_t now, uint32_t since, uint32_t duration) { return (uint32_t)(now - since) >= duration; }
  void startEvent(uint32_t now)
  {
    DeviceConfig &cfg = config::get();
    state.eventId++;
    state.totalEvents++;
    state.alarmCycle = 1;
    state.alarmActive = true;
    state.lastEventMs = now;
    alarmEndsMs = now + cfg.alarmDurationMs;
    if (cfg.audibleAlarm)
      buzzer::on();
    if (notifyCb)
      notifyCb(state.eventId, state.alarmCycle, false);
    if (eventCb)
      eventCb(state, false);
  }
  void closeEvent()
  {
    state.alarmActive = false;
    state.alarmCycle = 0;
    state.remainingAlarmMs = 0;
    buzzer::off();
    if (eventCb)
      eventCb(state, true);
  }
}

namespace intrusion_controller
{
  void begin(NotificationCallback notification, EventCallback event)
  {
    notifyCb = notification;
    eventCb = event;
    bootMs = millis();
    rawPrevious = pir_sensor::isMotionDetected();
    rawChangedMs = bootMs;
    state.rawPir = rawPrevious;
  }
  void update()
  {
    uint32_t now = millis();
    DeviceConfig &cfg = config::get();
    bool raw = pir_sensor::isMotionDetected();
    state.rawPir = raw;
    if (raw != rawPrevious)
    {
      rawPrevious = raw;
      rawChangedMs = now;
    }
    state.warmingUp = !elapsed(now, bootMs, cfg.pirWarmupMs);
    if (!state.warmingUp)
    {
      if (raw && !state.confirmedMotion && elapsed(now, rawChangedMs, cfg.motionConfirmMs))
        state.confirmedMotion = true;
      if (!raw && state.confirmedMotion && elapsed(now, rawChangedMs, cfg.clearConfirmMs))
        state.confirmedMotion = false;
      if (cfg.armed && state.confirmedMotion && !state.alarmActive)
        startEvent(now);
    }
    if (state.alarmActive)
    {
      state.remainingAlarmMs = (int32_t)(alarmEndsMs - now) > 0 ? alarmEndsMs - now : 0;
      if ((int32_t)(now - alarmEndsMs) >= 0)
      {
        if (state.confirmedMotion && cfg.repeatAlarm)
        {
          state.alarmCycle++;
          alarmEndsMs = now + cfg.alarmDurationMs;
          state.remainingAlarmMs = cfg.alarmDurationMs;
          if (cfg.audibleAlarm)
            buzzer::on();
          if (notifyCb)
            notifyCb(state.eventId, state.alarmCycle, false);
          if (eventCb)
            eventCb(state, false);
        }
        else
          closeEvent();
      }
    }
  }
  void simulateMotion()
  {
    // The physical PCB button deliberately follows the complete PIR event path.
    if (config::get().armed && !state.alarmActive)
      startEvent(millis());
  }
  void stopAudibleOutput() { buzzer::off(); }
  void testBuzzer(uint16_t ms) { buzzer::beep(ms); }
  const IntrusionStatus &status() { return state; }
}
