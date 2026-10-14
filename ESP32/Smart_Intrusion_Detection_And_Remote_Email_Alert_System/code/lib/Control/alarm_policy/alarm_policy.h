#pragma once

namespace alarm_policy {
  inline bool shouldStartEvent(bool armed, bool confirmedMotion, bool alarmActive, bool waitForConfirmedClear)
  {
    return armed && confirmedMotion && !alarmActive && !waitForConfirmedClear;
  }

  inline bool shouldRepeatAtBoundary(bool confirmedMotion, bool repeatAlarm)
  {
    return confirmedMotion && repeatAlarm;
  }

  inline bool mustClearBeforeAnotherEvent(bool confirmedMotion, bool repeatAlarm)
  {
    return confirmedMotion && !repeatAlarm;
  }
}
