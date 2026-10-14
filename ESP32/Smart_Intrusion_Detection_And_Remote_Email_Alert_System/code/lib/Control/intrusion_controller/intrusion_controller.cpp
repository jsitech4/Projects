#include "intrusion_controller.h"
#include "Storage/config/config.h"
#include "Sensors/pir_sensor/pir_sensor.h"
#include "Actuators/buzzer/buzzer.h"
#include "Communication/wifi_manager/wifi_manager.h"
#include "Control/alarm_policy/alarm_policy.h"

namespace
{
  IntrusionStatus state;
  uint32_t bootMs, rawChangedMs, alarmEndsMs;
  bool rawPrevious, waitForConfirmedClear;
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
    state.eventEpoch = wifi_manager::currentEpoch();
    alarmEndsMs = now + cfg.alarmDurationMs;
    if (cfg.audibleAlarm)
      buzzer::on();
    if (notifyCb)
      notifyCb(state.eventId, state.alarmCycle, state.eventEpoch, false);
    if (eventCb)
      eventCb(state, false);
  }
  void closeEvent()
  {
    state.alarmActive = false;
    state.remainingAlarmMs = 0;
    buzzer::off();
    if (eventCb)
      eventCb(state, true);
    state.alarmCycle = 0;
    if (alarm_policy::mustClearBeforeAnotherEvent(state.confirmedMotion, config::get().repeatAlarm))
      waitForConfirmedClear = true;
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
    waitForConfirmedClear = false;
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
      {
        state.confirmedMotion = false;
        waitForConfirmedClear = false;
      }
      if (alarm_policy::shouldStartEvent(cfg.armed, state.confirmedMotion, state.alarmActive, waitForConfirmedClear))
        startEvent(now);
    }
    if (state.alarmActive)
    {
      state.remainingAlarmMs = (int32_t)(alarmEndsMs - now) > 0 ? alarmEndsMs - now : 0;
      if ((int32_t)(now - alarmEndsMs) >= 0)
      {
        if (alarm_policy::shouldRepeatAtBoundary(state.confirmedMotion, cfg.repeatAlarm))
        {
          state.alarmCycle++;
          alarmEndsMs = now + cfg.alarmDurationMs;
          state.remainingAlarmMs = cfg.alarmDurationMs;
          if (cfg.audibleAlarm)
            buzzer::on();
          if (notifyCb)
            notifyCb(state.eventId, state.alarmCycle, state.eventEpoch, false);
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
