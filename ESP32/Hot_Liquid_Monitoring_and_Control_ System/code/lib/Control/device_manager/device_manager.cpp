#include <Arduino.h>
#include <math.h>

#include "device_manager.h"
#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "load_relay/load_relay.h"

namespace device_manager
{
  // ============================================================
  // Update timing
  // ============================================================

  static constexpr unsigned long UPDATE_INTERVAL_MS = 1000UL;

  // ============================================================
  // Default settings
  // ============================================================

  static constexpr float DEFAULT_FULL_DISTANCE_CM = 5.0f;
  static constexpr float DEFAULT_LOW_DISTANCE_CM = 40.0f;

  static constexpr float DEFAULT_FULL_LEVEL_PERCENT = 90.0f;
  static constexpr float DEFAULT_LOW_LEVEL_PERCENT = 20.0f;

  static constexpr float DEFAULT_LOW_TEMPERATURE_C = 30.0f;
  static constexpr float DEFAULT_HIGH_TEMPERATURE_C = 80.0f;

  // ============================================================
  // Configuration
  // ============================================================

  static float fullDistanceCm =
      DEFAULT_FULL_DISTANCE_CM;

  static float lowDistanceCm =
      DEFAULT_LOW_DISTANCE_CM;

  static float fullLevelPercent =
      DEFAULT_FULL_LEVEL_PERCENT;

  static float lowLevelPercent =
      DEFAULT_LOW_LEVEL_PERCENT;

  static float lowTemperatureC =
      DEFAULT_LOW_TEMPERATURE_C;

  static float highTemperatureC =
      DEFAULT_HIGH_TEMPERATURE_C;

  static RelayLatchMode relayLatchMode =
      LATCH_OFF;

  static TemperatureLatchMode temperatureLatchMode =
      TEMP_LATCH_OFF;

  // ============================================================
  // Sensor state
  // ============================================================

  static float temperatureC = NAN;
  static float distanceCm = NAN;
  static float levelPercent = NAN;

  static bool tempValid = false;
  static bool levelValid = false;

  /*
   * The current project does not yet expose a vibration sensor
   * state through the device_manager API.
   *
   * Keep this field available in Snapshot and report it as false
   * until the vibration module is integrated.
   */
  static bool vibrationReady = false;

  static TemperatureStatus temperatureStatus =
      TEMP_STATUS_INVALID;

  static TankStatus tankStatus =
      TANK_STATUS_INVALID;

  // ============================================================
  // Automatic latch state
  // ============================================================

  static bool tankLatchTriggered = false;
  static bool temperatureLatchTriggered = false;

  // ============================================================
  // Relay control state
  // ============================================================

  /*
   * Manual request:
   *
   *     false = user requested relay OFF
   *     true  = user requested relay ON
   *
   * This is intentionally separate from the physical relay
   * state.
   */
  static bool manualRelayRequest = false;

  /*
   * True when one or more automatic conditions currently demand
   * the relay.
   */
  static bool automaticRelayActive = false;

  // ============================================================
  // Timing
  // ============================================================

  static unsigned long lastUpdateMs = 0;

  // ============================================================
  // Snapshot
  // ============================================================

  static Snapshot snapshot{};

  // ============================================================
  // Utility
  // ============================================================

  static bool isFiniteFloat(float value)
  {
    return isfinite(value);
  }

  // ============================================================
  // Relay decision
  // ============================================================

  /*
   * The effective relay state is:
   *
   *     Manual request OR automatic demand
   *
   * Truth table:
   *
   * Manual   Automatic   Relay
   * --------------------------
   * OFF      OFF         OFF
   * ON       OFF         ON
   * OFF      ON          ON
   * ON       ON          ON
   *
   * This is the single relay decision point in this module.
   */
  static void applyRelayDecision()
  {
    const bool automaticDemand =
        tankLatchTriggered ||
        temperatureLatchTriggered;

    automaticRelayActive =
        automaticDemand;

    const bool desiredRelayState =
        manualRelayRequest ||
        automaticDemand;

    if (load_relay::isOn() != desiredRelayState)
    {
      load_relay::setOn(desiredRelayState);
    }
  }

  // ============================================================
  // Tank status
  // ============================================================

  static void updateTankStatus()
  {
    if (!levelValid ||
        !isFiniteFloat(levelPercent))
    {
      tankStatus =
          TANK_STATUS_INVALID;

      return;
    }

    if (levelPercent >= fullLevelPercent)
    {
      tankStatus =
          TANK_STATUS_FULL;
    }
    else if (levelPercent <= lowLevelPercent)
    {
      tankStatus =
          TANK_STATUS_LOW;
    }
    else
    {
      tankStatus =
          TANK_STATUS_NORMAL;
    }
  }

  // ============================================================
  // Temperature status
  // ============================================================

  static void updateTemperatureStatus()
  {
    if (!tempValid ||
        !isFiniteFloat(temperatureC))
    {
      temperatureStatus =
          TEMP_STATUS_INVALID;

      return;
    }

    if (temperatureC < lowTemperatureC)
    {
      temperatureStatus =
          TEMP_STATUS_LOW;
    }
    else if (temperatureC > highTemperatureC)
    {
      temperatureStatus =
          TEMP_STATUS_HIGH;
    }
    else
    {
      temperatureStatus =
          TEMP_STATUS_NORMAL;
    }
  }

  // ============================================================
  // Tank automatic latch
  // ============================================================

  static void updateTankLatch()
  {
    if (relayLatchMode == LATCH_OFF)
    {
      tankLatchTriggered = false;
      return;
    }

    /*
     * Invalid sensor data does not clear an existing latch.
     */
    if (!levelValid ||
        !isFiniteFloat(levelPercent))
    {
      return;
    }

    if (relayLatchMode == LATCH_AT_FULL)
    {
      tankLatchTriggered =
          (levelPercent >= fullLevelPercent);
    }
    else if (relayLatchMode == LATCH_AT_LOW)
    {
      tankLatchTriggered =
          (levelPercent <= lowLevelPercent);
    }
    else
    {
      tankLatchTriggered = false;
    }
  }

  // ============================================================
  // Temperature automatic latch
  // ============================================================

  static void updateTemperatureLatch()
  {
    if (temperatureLatchMode == TEMP_LATCH_OFF)
    {
      temperatureLatchTriggered = false;
      return;
    }

    /*
     * Invalid sensor data does not clear an existing latch.
     */
    if (!tempValid ||
        !isFiniteFloat(temperatureC))
    {
      return;
    }

    if (temperatureLatchMode == TEMP_LATCH_AT_LOW)
    {
      temperatureLatchTriggered =
          (temperatureC < lowTemperatureC);
    }
    else if (temperatureLatchMode == TEMP_LATCH_AT_HIGH)
    {
      temperatureLatchTriggered =
          (temperatureC > highTemperatureC);
    }
    else
    {
      temperatureLatchTriggered = false;
    }
  }

  // ============================================================
  // Automatic relay update
  // ============================================================

  static void updateAutomaticRelay()
  {
    applyRelayDecision();
  }

  // ============================================================
  // Snapshot
  // ============================================================

  static void updateSnapshot()
  {
    snapshot.uptimeMs =
        millis();

    snapshot.temperatureC =
        temperatureC;

    snapshot.distanceCm =
        distanceCm;

    snapshot.levelPercent =
        levelPercent;

    snapshot.tempValid =
        tempValid;

    snapshot.levelValid =
        levelValid;

    snapshot.vibrationReady =
        vibrationReady;

    /*
     * Actual effective relay state.
     */
    snapshot.relayOn =
        load_relay::isOn();

    /*
     * Effective request:
     *
     * Manual request OR automatic demand.
     */
    snapshot.relayRequested =
        manualRelayRequest ||
        automaticRelayActive;

    /*
     * User's manual request only.
     */
    snapshot.manualRelayRequest =
        manualRelayRequest;

    snapshot.temperatureStatus =
        temperatureStatus;

    snapshot.tankStatus =
        tankStatus;

    snapshot.tankLatchTriggered =
        tankLatchTriggered;

    snapshot.temperatureLatchTriggered =
        temperatureLatchTriggered;

    snapshot.automaticRelayDemand =
        automaticRelayActive;

    snapshot.fullDistanceCm =
        fullDistanceCm;

    snapshot.lowDistanceCm =
        lowDistanceCm;

    snapshot.fullLevelPercent =
        fullLevelPercent;

    snapshot.lowLevelPercent =
        lowLevelPercent;

    snapshot.lowTemperatureC =
        lowTemperatureC;

    snapshot.highTemperatureC =
        highTemperatureC;

    snapshot.relayLatchMode =
        relayLatchMode;

    snapshot.temperatureLatchMode =
        temperatureLatchMode;
  }

  // ============================================================
  // Begin
  // ============================================================

  void begin()
  {
    // ----------------------------------------------------------
    // Configuration defaults
    // ----------------------------------------------------------

    fullDistanceCm =
        DEFAULT_FULL_DISTANCE_CM;

    lowDistanceCm =
        DEFAULT_LOW_DISTANCE_CM;

    fullLevelPercent =
        DEFAULT_FULL_LEVEL_PERCENT;

    lowLevelPercent =
        DEFAULT_LOW_LEVEL_PERCENT;

    lowTemperatureC =
        DEFAULT_LOW_TEMPERATURE_C;

    highTemperatureC =
        DEFAULT_HIGH_TEMPERATURE_C;

    relayLatchMode =
        LATCH_OFF;

    temperatureLatchMode =
        TEMP_LATCH_OFF;

    // ----------------------------------------------------------
    // Sensor state
    // ----------------------------------------------------------

    temperatureC = NAN;
    distanceCm = NAN;
    levelPercent = NAN;

    tempValid = false;
    levelValid = false;

    vibrationReady = false;

    temperatureStatus =
        TEMP_STATUS_INVALID;

    tankStatus =
        TANK_STATUS_INVALID;

    // ----------------------------------------------------------
    // Automatic states
    // ----------------------------------------------------------

    tankLatchTriggered = false;
    temperatureLatchTriggered = false;

    automaticRelayActive = false;

    // ----------------------------------------------------------
    // Manual relay state
    // ----------------------------------------------------------

    manualRelayRequest = false;

    // ----------------------------------------------------------
    // Timing
    // ----------------------------------------------------------

    lastUpdateMs = 0;

    // ----------------------------------------------------------
    // Initial relay state
    // ----------------------------------------------------------

    load_relay::turnOff();

    // ----------------------------------------------------------
    // Initial snapshot
    // ----------------------------------------------------------

    updateSnapshot();
  }

  // ============================================================
  // Update
  // ============================================================

  void update()
  {
    const unsigned long now =
        millis();

    if ((unsigned long)(now - lastUpdateMs) <
        UPDATE_INTERVAL_MS)
    {
      return;
    }

    lastUpdateMs =
        now;

    // ----------------------------------------------------------
    // Temperature
    // ----------------------------------------------------------

    temperatureC =
        temp_sensor::getTemperatureC();

    tempValid =
        temp_sensor::isValid();

    // ----------------------------------------------------------
    // Ultrasonic distance
    // ----------------------------------------------------------

    distanceCm =
        ultrasonic_sensor::getDistanceCm();

    levelValid =
        ultrasonic_sensor::isValid();

    // ----------------------------------------------------------
    // Level calculation
    // ----------------------------------------------------------

    if (levelValid &&
        isFiniteFloat(distanceCm) &&
        lowDistanceCm > fullDistanceCm)
    {
      float calculatedLevel =
          ((lowDistanceCm - distanceCm) /
           (lowDistanceCm - fullDistanceCm)) *
          100.0f;

      if (calculatedLevel < 0.0f)
      {
        calculatedLevel = 0.0f;
      }

      if (calculatedLevel > 100.0f)
      {
        calculatedLevel = 100.0f;
      }

      levelPercent =
          calculatedLevel;
    }
    else
    {
      levelPercent =
          NAN;
    }

    // ----------------------------------------------------------
    // Status calculations
    // ----------------------------------------------------------

    updateTankStatus();
    updateTemperatureStatus();

    // ----------------------------------------------------------
    // Automatic latch calculations
    // ----------------------------------------------------------

    updateTankLatch();
    updateTemperatureLatch();

    // ----------------------------------------------------------
    // Relay
    // ----------------------------------------------------------

    updateAutomaticRelay();

    // ----------------------------------------------------------
    // Snapshot
    // ----------------------------------------------------------

    updateSnapshot();
  }

  // ============================================================
  // Get snapshot
  // ============================================================

  Snapshot getSnapshot()
  {
    updateSnapshot();

    return snapshot;
  }

  // ============================================================
  // Level
  // ============================================================

  float getLevelPercent()
  {
    return levelPercent;
  }

  void setLevelThresholds(
      float fullPercent,
      float lowPercent)
  {
    if (!isFiniteFloat(fullPercent) ||
        !isFiniteFloat(lowPercent))
    {
      return;
    }

    if (fullPercent <= lowPercent)
    {
      return;
    }

    if (lowPercent < 0.0f ||
        fullPercent > 100.0f)
    {
      return;
    }

    fullLevelPercent =
        fullPercent;

    lowLevelPercent =
        lowPercent;

    updateTankStatus();
    updateTankLatch();

    applyRelayDecision();

    updateSnapshot();
  }

  float getFullLevelPercent()
  {
    return fullLevelPercent;
  }

  float getLowLevelPercent()
  {
    return lowLevelPercent;
  }

  // ============================================================
  // Tank distances
  // ============================================================

  bool setTankDistances(
      float newFullDistanceCm,
      float newLowDistanceCm)
  {
    if (!isFiniteFloat(newFullDistanceCm) ||
        !isFiniteFloat(newLowDistanceCm))
    {
      return false;
    }

    if (newFullDistanceCm >= newLowDistanceCm)
    {
      return false;
    }

    if (newFullDistanceCm < 0.0f ||
        newLowDistanceCm <= 0.0f)
    {
      return false;
    }

    fullDistanceCm =
        newFullDistanceCm;

    lowDistanceCm =
        newLowDistanceCm;

    return true;
  }

  float getFullDistanceCm()
  {
    return fullDistanceCm;
  }

  float getLowDistanceCm()
  {
    return lowDistanceCm;
  }

  // ============================================================
  // Temperature thresholds
  // ============================================================

  bool setTemperatureThresholds(
      float newLowTemperatureC,
      float newHighTemperatureC)
  {
    if (!isFiniteFloat(newLowTemperatureC) ||
        !isFiniteFloat(newHighTemperatureC))
    {
      return false;
    }

    if (newLowTemperatureC >= newHighTemperatureC)
    {
      return false;
    }

    lowTemperatureC =
        newLowTemperatureC;

    highTemperatureC =
        newHighTemperatureC;

    updateTemperatureStatus();
    updateTemperatureLatch();

    applyRelayDecision();

    updateSnapshot();

    return true;
  }

  float getLowTemperatureC()
  {
    return lowTemperatureC;
  }

  float getHighTemperatureC()
  {
    return highTemperatureC;
  }

  TemperatureStatus getTemperatureStatus()
  {
    return temperatureStatus;
  }

  const char *getTemperatureStatusText()
  {
    switch (temperatureStatus)
    {
    case TEMP_STATUS_LOW:
      return "LOW";

    case TEMP_STATUS_NORMAL:
      return "NORMAL";

    case TEMP_STATUS_HIGH:
      return "HIGH";

    case TEMP_STATUS_INVALID:
    default:
      return "INVALID";
    }
  }

  // ============================================================
  // Tank relay latch
  // ============================================================

  void setRelayLatchMode(
      RelayLatchMode mode)
  {
    if (mode != LATCH_OFF &&
        mode != LATCH_AT_FULL &&
        mode != LATCH_AT_LOW)
    {
      mode =
          LATCH_OFF;
    }

    relayLatchMode =
        mode;

    updateTankLatch();

    applyRelayDecision();

    updateSnapshot();
  }

  RelayLatchMode getRelayLatchMode()
  {
    return relayLatchMode;
  }

  bool isTankLatchTriggered()
  {
    return tankLatchTriggered;
  }

  // ============================================================
  // Temperature relay latch
  // ============================================================

  void setTemperatureLatchMode(
      TemperatureLatchMode mode)
  {
    if (mode != TEMP_LATCH_OFF &&
        mode != TEMP_LATCH_AT_LOW &&
        mode != TEMP_LATCH_AT_HIGH)
    {
      mode =
          TEMP_LATCH_OFF;
    }

    temperatureLatchMode =
        mode;

    updateTemperatureLatch();

    applyRelayDecision();

    updateSnapshot();
  }

  TemperatureLatchMode getTemperatureLatchMode()
  {
    return temperatureLatchMode;
  }

  bool isTemperatureLatchTriggered()
  {
    return temperatureLatchTriggered;
  }

  // ============================================================
  // Compatibility API
  // ============================================================

  void setTemperatureRelayLatchMode(
      TemperatureLatchMode mode)
  {
    setTemperatureLatchMode(mode);
  }

  TemperatureLatchMode getTemperatureRelayLatchMode()
  {
    return getTemperatureLatchMode();
  }

  // ============================================================
  // Manual relay request
  // ============================================================

  void setManualRelayRequest(bool state)
  {
    /*
     * Store the manual request.
     */
    manualRelayRequest =
        state;

    /*
     * Immediately recalculate the effective relay state.
     */
    applyRelayDecision();

    updateSnapshot();
  }

  bool getManualRelayRequest()
  {
    return manualRelayRequest;
  }

  // ============================================================
  // Automatic relay demand
  // ============================================================

  bool isAutomaticRelayDemand()
  {
    return tankLatchTriggered ||
           temperatureLatchTriggered;
  }

  // ============================================================
  // Apply settings
  // ============================================================

  bool applySettings(
      float newFullDistanceCm,
      float newLowDistanceCm,
      float newLowTemperatureC,
      float newHighTemperatureC,
      RelayLatchMode newTankLatchMode,
      TemperatureLatchMode newTemperatureLatchMode)
  {
    // ----------------------------------------------------------
    // Validate distances
    // ----------------------------------------------------------

    if (!isFiniteFloat(newFullDistanceCm) ||
        !isFiniteFloat(newLowDistanceCm))
    {
      return false;
    }

    if (newFullDistanceCm >= newLowDistanceCm)
    {
      return false;
    }

    if (newFullDistanceCm < 0.0f ||
        newLowDistanceCm <= 0.0f)
    {
      return false;
    }

    // ----------------------------------------------------------
    // Validate temperatures
    // ----------------------------------------------------------

    if (!isFiniteFloat(newLowTemperatureC) ||
        !isFiniteFloat(newHighTemperatureC))
    {
      return false;
    }

    if (newLowTemperatureC >= newHighTemperatureC)
    {
      return false;
    }

    // ----------------------------------------------------------
    // Validate tank latch
    // ----------------------------------------------------------

    if (newTankLatchMode != LATCH_OFF &&
        newTankLatchMode != LATCH_AT_FULL &&
        newTankLatchMode != LATCH_AT_LOW)
    {
      return false;
    }

    // ----------------------------------------------------------
    // Validate temperature latch
    // ----------------------------------------------------------

    if (newTemperatureLatchMode != TEMP_LATCH_OFF &&
        newTemperatureLatchMode != TEMP_LATCH_AT_LOW &&
        newTemperatureLatchMode != TEMP_LATCH_AT_HIGH)
    {
      return false;
    }

    // ----------------------------------------------------------
    // Apply settings
    // ----------------------------------------------------------

    fullDistanceCm =
        newFullDistanceCm;

    lowDistanceCm =
        newLowDistanceCm;

    lowTemperatureC =
        newLowTemperatureC;

    highTemperatureC =
        newHighTemperatureC;

    relayLatchMode =
        newTankLatchMode;

    temperatureLatchMode =
        newTemperatureLatchMode;

    // ----------------------------------------------------------
    // Recalculate status
    // ----------------------------------------------------------

    updateTankStatus();
    updateTemperatureStatus();

    updateTankLatch();
    updateTemperatureLatch();

    /*
     * IMPORTANT:
     *
     * Do not change manualRelayRequest here.
     */
    applyRelayDecision();

    updateSnapshot();

    return true;
  }

  // ============================================================
  // Clear automatic latches
  // ============================================================

  void clearAutomaticLatches()
  {
    tankLatchTriggered =
        false;

    temperatureLatchTriggered =
        false;

    /*
     * Do not blindly turn the relay OFF.
     *
     * The manual request may still be ON.
     */
    applyRelayDecision();

    updateSnapshot();
  }
}
