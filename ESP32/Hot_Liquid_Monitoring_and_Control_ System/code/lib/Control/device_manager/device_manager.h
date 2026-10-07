#pragma once

#include <Arduino.h>

namespace device_manager
{
  // ============================================================
  // Tank relay latch modes
  // ============================================================

  enum RelayLatchMode : uint8_t
  {
    LATCH_OFF = 0,
    LATCH_AT_FULL = 1,
    LATCH_AT_LOW = 2
  };

  // ============================================================
  // Temperature relay latch modes
  // ============================================================

  enum TemperatureLatchMode : uint8_t
  {
    TEMP_LATCH_OFF = 0,
    TEMP_LATCH_AT_LOW = 1,
    TEMP_LATCH_AT_HIGH = 2
  };

  // ============================================================
  // Temperature status
  // ============================================================

  enum TemperatureStatus : uint8_t
  {
    TEMP_STATUS_INVALID = 0,
    TEMP_STATUS_LOW = 1,
    TEMP_STATUS_NORMAL = 2,
    TEMP_STATUS_HIGH = 3
  };

  // ============================================================
  // Tank status
  // ============================================================

  enum TankStatus : uint8_t
  {
    TANK_STATUS_INVALID = 0,
    TANK_STATUS_LOW = 1,
    TANK_STATUS_NORMAL = 2,
    TANK_STATUS_FULL = 3
  };

  // ============================================================
  // System snapshot
  // ============================================================

  struct Snapshot
  {
    // ----------------------------------------------------------
    // General
    // ----------------------------------------------------------

    unsigned long uptimeMs;

    // ----------------------------------------------------------
    // Sensor data
    // ----------------------------------------------------------

    float temperatureC;
    float distanceCm;
    float levelPercent;

    bool tempValid;
    bool levelValid;
    bool vibrationReady;

    // ----------------------------------------------------------
    // Relay state
    // ----------------------------------------------------------

    // Actual commanded relay state.
    bool relayOn;

    // Effective relay request after combining:
    // manual request OR automatic demand.
    bool relayRequested;

    // User's persistent manual relay request.
    bool manualRelayRequest;

    // ----------------------------------------------------------
    // Status
    // ----------------------------------------------------------

    TemperatureStatus temperatureStatus;
    TankStatus tankStatus;

    // ----------------------------------------------------------
    // Automatic latch state
    // ----------------------------------------------------------

    bool tankLatchTriggered;
    bool temperatureLatchTriggered;

    // A threshold fault remains latched until clearFault succeeds,
    // including after readings recover or settings are changed.
    bool faultLatched;

    // True when a fault exists and the latest sampled readings
    // allow it to be cleared. clearFault rechecks current inputs.
    bool faultClearable;

    // True while one or more automatic faults remain latched
    // and demand the relay to be ON.
    bool automaticRelayDemand;

    // ----------------------------------------------------------
    // Tank configuration
    // ----------------------------------------------------------

    float fullDistanceCm;
    float lowDistanceCm;

    float fullLevelPercent;
    float lowLevelPercent;

    // ----------------------------------------------------------
    // Temperature configuration
    // ----------------------------------------------------------

    float lowTemperatureC;
    float highTemperatureC;

    // ----------------------------------------------------------
    // Latch configuration
    // ----------------------------------------------------------

    RelayLatchMode relayLatchMode;
    TemperatureLatchMode temperatureLatchMode;
  };

  // ============================================================
  // Lifecycle
  // ============================================================

  void begin();
  void update();

  // ============================================================
  // Snapshot
  // ============================================================

  Snapshot getSnapshot();

  // ============================================================
  // Level
  // ============================================================

  float getLevelPercent();

  void setLevelThresholds(
      float fullPercent,
      float lowPercent);

  float getFullLevelPercent();
  float getLowLevelPercent();

  // ============================================================
  // Tank distance
  // ============================================================

  bool setTankDistances(
      float fullDistanceCm,
      float lowDistanceCm);

  float getFullDistanceCm();
  float getLowDistanceCm();

  // ============================================================
  // Temperature
  // ============================================================

  bool setTemperatureThresholds(
      float lowTemperatureC,
      float highTemperatureC);

  float getLowTemperatureC();
  float getHighTemperatureC();

  TemperatureStatus getTemperatureStatus();

  const char *getTemperatureStatusText();

  // ============================================================
  // Tank relay latch configuration
  // ============================================================

  void setRelayLatchMode(
      RelayLatchMode mode);

  RelayLatchMode getRelayLatchMode();

  bool isTankLatchTriggered();

  // ============================================================
  // Temperature relay latch configuration
  // ============================================================

  void setTemperatureLatchMode(
      TemperatureLatchMode mode);

  TemperatureLatchMode getTemperatureLatchMode();

  bool isTemperatureLatchTriggered();

  // ============================================================
  // Compatibility temperature latch API
  // ============================================================

  void setTemperatureRelayLatchMode(
      TemperatureLatchMode mode);

  TemperatureLatchMode getTemperatureRelayLatchMode();

  // ============================================================
  // Manual relay control
  // ============================================================

  // Stores the user's manual ON/OFF request separately from
  // the automatic relay demand.
  //
  // Effective relay state:
  //
  //     manualRelayRequest OR automaticRelayDemand
  //
  // This prevents an automatic event from destroying a
  // previous manual ON request.
  void setManualRelayRequest(bool state);

  bool getManualRelayRequest();

  // ============================================================
  // Automatic relay state
  // ============================================================

  bool isAutomaticRelayDemand();

  // ============================================================
  // Apply complete configuration
  // ============================================================

  bool applySettings(
      float fullDistanceCm,
      float lowDistanceCm,
      float lowTemperatureC,
      float highTemperatureC,
      RelayLatchMode tankLatchMode,
      TemperatureLatchMode temperatureLatchMode);

  // ============================================================
  // Clear automatic latches
  // ============================================================

  // True while either threshold fault remains latched.
  bool hasFault();

  // Refreshes the latest sensor inputs without the update timer.
  // Clears all faults together only when no enabled threshold is
  // active and every latched source has a valid, finite reading.
  // Returns false if clearing is blocked; preserves manual relay
  // requests. Settings changes never clear faults by themselves.
  bool clearFault();

  // Compatibility wrapper using the same guarded clearFault
  // operation; active faults cannot be bypassed through this API.
  void clearAutomaticLatches();
}
