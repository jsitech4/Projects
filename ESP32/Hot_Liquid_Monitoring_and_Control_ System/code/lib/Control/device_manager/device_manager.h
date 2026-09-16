#pragma once

#include <Arduino.h>

namespace device_manager
{
  enum RelayLatchMode : uint8_t
  {
    LATCH_OFF = 0,
    LATCH_AT_FULL = 1,
    LATCH_AT_LOW = 2
  };

  enum TemperatureLatchMode : uint8_t
  {
    TEMP_LATCH_OFF = 0,
    TEMP_LATCH_LOW = 1,
    TEMP_LATCH_HIGH = 2
  };

  enum TemperatureStatus : uint8_t
  {
    TEMP_STATUS_INVALID = 0,
    TEMP_STATUS_LOW = 1,
    TEMP_STATUS_NORMAL = 2,
    TEMP_STATUS_HIGH = 3
  };

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

    bool tankLatchTriggered;
    bool temperatureLatchTriggered;
  };

  void begin();
  void update();

  Snapshot getSnapshot();

  // ---------------------------------------------------------
  // Tank level
  // ---------------------------------------------------------

  float getLevelPercent();

  float getFullDistanceCm();
  float getLowDistanceCm();

  bool setTankDistances(float fullDistanceCm,
                        float lowDistanceCm);

  // Existing percentage thresholds are retained.
  float getFullLevelPercent();
  float getLowLevelPercent();

  bool setLevelThresholds(float fullPercent,
                          float lowPercent);

  // ---------------------------------------------------------
  // Tank relay latch
  // ---------------------------------------------------------

  void setRelayLatchMode(RelayLatchMode mode);
  RelayLatchMode getRelayLatchMode();

  // ---------------------------------------------------------
  // Temperature
  // ---------------------------------------------------------

  float getLowTemperatureC();
  float getHighTemperatureC();

  bool setTemperatureThresholds(float lowTemperatureC,
                                float highTemperatureC);

  TemperatureStatus getTemperatureStatus();

  const char *getTemperatureStatusText();

  // ---------------------------------------------------------
  // Temperature relay latch
  // ---------------------------------------------------------

  void setTemperatureLatchMode(TemperatureLatchMode mode);
  TemperatureLatchMode getTemperatureLatchMode();

  // ---------------------------------------------------------
  // Combined settings
  // ---------------------------------------------------------

  bool applySettings(
      float fullDistanceCm,
      float lowDistanceCm,
      float fullLevelPercent,
      float lowLevelPercent,
      RelayLatchMode tankLatchMode,
      float lowTemperatureC,
      float highTemperatureC,
      TemperatureLatchMode temperatureLatchMode);
}
