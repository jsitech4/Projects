#pragma once

#include <stdint.h>

namespace status_indicator_logic {
  enum class Mode : uint8_t {
    Intrusion, CriticalBattery, LowBattery, SystemFault, WifiDisconnected,
    InternetUnavailable, WarmingUp, Provisioning, Healthy
  };

  inline uint8_t batteryBars(bool enabled, uint8_t percent)
  {
    if (!enabled || percent == 0) return 0;
    uint8_t bars = (percent + 24) / 25;
    return bars > 4 ? 4 : bars;
  }

  inline Mode selectMode(bool intrusionActive, bool batteryCritical, bool batteryLow,
                         bool systemFault, bool wifiConnected, bool internetAvailable,
                         bool warmingUp, bool provisioning)
  {
    if (intrusionActive) return Mode::Intrusion;
    if (batteryCritical) return Mode::CriticalBattery;
    if (batteryLow) return Mode::LowBattery;
    if (systemFault) return Mode::SystemFault;
    if (!wifiConnected) return Mode::WifiDisconnected;
    if (!internetAvailable) return Mode::InternetUnavailable;
    if (warmingUp) return Mode::WarmingUp;
    if (provisioning) return Mode::Provisioning;
    return Mode::Healthy;
  }

  inline bool pulseTrain(uint32_t now, uint32_t periodMs, uint32_t onMs, uint8_t count, uint32_t gapMs)
  {
    uint32_t phase = now % periodMs;
    for (uint8_t pulse = 0; pulse < count; ++pulse) {
      uint32_t start = pulse * (onMs + gapMs);
      if (phase >= start && phase < start + onMs) return true;
    }
    return false;
  }

  inline bool isOn(Mode mode, uint32_t now)
  {
    switch (mode) {
      case Mode::Intrusion: return pulseTrain(now, 1400, 90, 3, 90);
      case Mode::CriticalBattery: return pulseTrain(now, 1800, 120, 4, 90);
      case Mode::LowBattery: return pulseTrain(now, 2000, 180, 2, 140);
      case Mode::SystemFault: return pulseTrain(now, 1000, 150, 2, 100);
      case Mode::WifiDisconnected: return pulseTrain(now, 2000, 600, 1, 0);
      case Mode::InternetUnavailable: return pulseTrain(now, 2000, 120, 2, 120);
      case Mode::WarmingUp: return pulseTrain(now, 1000, 100, 1, 0);
      case Mode::Provisioning: return pulseTrain(now, 1000, 100, 2, 120);
      case Mode::Healthy: return pulseTrain(now, 2000, 100, 1, 0);
    }
    return false;
  }
}
