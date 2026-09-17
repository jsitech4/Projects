#pragma once

#include <Arduino.h>

namespace device_manager
{
  // =========================================================================
  // Tank relay latch modes
  // =========================================================================

  enum RelayLatchMode : uint8_t
  {
    LATCH_OFF = 0,
    LATCH_AT_FULL = 1,
    LATCH_AT_LOW = 2
  };

  // =========================================================================
  // Temperature relay latch modes
  // =========================================================================

  enum TemperatureLatchMode : uint8_t
  {
    TEMP_LATCH_OFF = 0,
    TEMP_LATCH_AT_LOW = 1,
    TEMP_LATCH_AT_HIGH = 2
  };

  // =========================================================================
  // Temperature status
  // =========================================================================

  enum TemperatureStatus : uint8_t
  {
    TEMP_STATUS_INVALID = 0,
    TEMP_STATUS_LOW = 1,
    TEMP_STATUS_NORMAL = 2,
    TEMP_STATUS_HIGH = 3
  };

  // =========================================================================
  // Tank status
  // =========================================================================

  enum TankStatus : uint8_t
  {
    TANK_STATUS_INVALID = 0,
    TANK_STATUS_LOW = 1,
    TANK_STATUS_NORMAL = 2,
    TANK_STATUS_FULL = 3
  };

  // =========================================================================
  // Device snapshot
  // =========================================================================

  struct Snapshot
  {
    unsigned long uptimeMs;

    float temperatureC;
    float distanceCm;
    float levelPercent;

    bool tempValid;
    bool levelValid;

    bool vibrationReady;

    bool relayOn;
    bool relayRequested;

    // Current conditions
    TemperatureStatus temperatureStatus;
    TankStatus tankStatus;

    // Automatic latch states
    //
    // These names are kept exactly as expected by your existing main.cpp.
    bool tankLatchTriggered;
    bool temperatureLatchTriggered;

    // Combined automatic demand
    bool automaticRelayDemand;

    // Tank calibration
    float fullDistanceCm;
    float lowDistanceCm;

    // Temperature thresholds
    float lowTemperatureC;
    float highTemperatureC;

    // Legacy level percentages
    float fullLevelPercent;
    float lowLevelPercent;

    // Automation modes
    RelayLatchMode relayLatchMode;
    TemperatureLatchMode temperatureLatchMode;
  };

  // =========================================================================
  // Lifecycle
  // =========================================================================

  void begin();
  void update();

  // =========================================================================
  // Snapshot
  // =========================================================================

  Snapshot getSnapshot();

  // =========================================================================
  // Tank level
  // =========================================================================

  float getLevelPercent();

  // Legacy percentage threshold API
  void setLevelThresholds(float fullPercent,
                          float lowPercent);

  float getFullLevelPercent();
  float getLowLevelPercent();

  // Physical ultrasonic calibration
  bool setTankDistances(float fullDistanceCm,
                        float lowDistanceCm);

  float getFullDistanceCm();
  float getLowDistanceCm();

  // =========================================================================
  // Temperature
  // =========================================================================

  bool setTemperatureThresholds(float lowTemperatureC,
                                float highTemperatureC);

  float getLowTemperatureC();
  float getHighTemperatureC();

  TemperatureStatus getTemperatureStatus();

  // Text representation used by main.cpp/dashboard
  const char *getTemperatureStatusText();

  // =========================================================================
  // Tank relay latch
  // =========================================================================

  void setRelayLatchMode(RelayLatchMode mode);

  RelayLatchMode getRelayLatchMode();

  bool isTankLatchTriggered();

  // =========================================================================
  // Temperature relay latch
  // =========================================================================

  void setTemperatureLatchMode(TemperatureLatchMode mode);

  TemperatureLatchMode getTemperatureLatchMode();

  bool isTemperatureLatchTriggered();

  // Compatibility aliases
  void setTemperatureRelayLatchMode(TemperatureLatchMode mode);

  TemperatureLatchMode getTemperatureRelayLatchMode();

  // =========================================================================
  // Automatic relay
  // =========================================================================

  bool isAutomaticRelayDemand();

  // =========================================================================
  // Apply all settings at once
  // =========================================================================

  bool applySettings(
      float fullDistanceCm,
      float lowDistanceCm,
      float lowTemperatureC,
      float highTemperatureC,
      RelayLatchMode tankLatchMode,
      TemperatureLatchMode temperatureLatchMode);

  // =========================================================================
  // Clear automatic latch states
  // =========================================================================

  void clearAutomaticLatches();
}
