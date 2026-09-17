#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "device_manager.h"

#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "load_relay/load_relay.h"

namespace device_manager
{
  // =========================================================================
  // Default configuration
  // =========================================================================

  // Top-mounted ultrasonic sensor:
  //
  // Smaller distance = more liquid
  // Larger distance  = less liquid
  //
  // Example:
  //      5 cm  = full
  //     40 cm  = low/empty
  //
  static float fullDistanceCm = 5.0f;
  static float lowDistanceCm = 40.0f;

  // Legacy percentage thresholds.
  //
  // These remain available because your existing dashboard/main.cpp uses
  // them. Physical level is calculated from the distance calibration.
  static float fullLevelPercent = 90.0f;
  static float lowLevelPercent = 20.0f;

  // =========================================================================
  // Temperature configuration
  // =========================================================================

  static float lowTemperatureC = 30.0f;
  static float highTemperatureC = 80.0f;

  // =========================================================================
  // Automation modes
  // =========================================================================

  static RelayLatchMode tankLatchMode = LATCH_OFF;

  static TemperatureLatchMode temperatureLatchMode =
      TEMP_LATCH_OFF;

  // =========================================================================
  // Runtime latch states
  // =========================================================================

  static bool tankLatchTriggered = false;

  static bool temperatureLatchTriggered = false;

  // True only when the device manager has automatically turned the relay ON.
  //
  // This is important because manual dashboard relay control should not be
  // confused with automatic relay control.
  static bool automaticRelayActive = false;

  // =========================================================================
  // Snapshot
  // =========================================================================

  static Snapshot snap;

  static unsigned long lastUpdate = 0;

  static const unsigned long UPDATE_INTERVAL_MS = 1000UL;

  // =========================================================================
  // Utility
  // =========================================================================

  static bool isValidNumber(float value)
  {
    return !isnan(value) && !isinf(value);
  }

  static float clamp(float value,
                     float minimum,
                     float maximum)
  {
    if (value < minimum)
      return minimum;

    if (value > maximum)
      return maximum;

    return value;
  }

  // =========================================================================
  // Tank level calculation
  // =========================================================================

  static float calculateLevel(float distanceCm)
  {
    if (!isValidNumber(distanceCm))
      return NAN;

    if (distanceCm <= 0.0f)
      return NAN;

    if (lowDistanceCm <= fullDistanceCm)
      return NAN;

    const float level =
        ((lowDistanceCm - distanceCm) * 100.0f) /
        (lowDistanceCm - fullDistanceCm);

    return clamp(level, 0.0f, 100.0f);
  }

  // =========================================================================
  // Tank status
  // =========================================================================

  static TankStatus calculateTankStatus(float levelPercent)
  {
    if (!isValidNumber(levelPercent))
      return TANK_STATUS_INVALID;

    if (levelPercent >= fullLevelPercent)
      return TANK_STATUS_FULL;

    if (levelPercent <= lowLevelPercent)
      return TANK_STATUS_LOW;

    return TANK_STATUS_NORMAL;
  }

  // =========================================================================
  // Temperature status
  // =========================================================================

  static TemperatureStatus calculateTemperatureStatus(
      float temperatureC)
  {
    if (!isValidNumber(temperatureC))
      return TEMP_STATUS_INVALID;

    if (temperatureC < lowTemperatureC)
      return TEMP_STATUS_LOW;

    if (temperatureC > highTemperatureC)
      return TEMP_STATUS_HIGH;

    return TEMP_STATUS_NORMAL;
  }

  // =========================================================================
  // Tank latch
  // =========================================================================

  static void updateTankLatch()
  {
    // If automation is disabled, there can be no active tank latch.
    if (tankLatchMode == LATCH_OFF)
    {
      tankLatchTriggered = false;
      return;
    }

    // Never reset an active latch because of an invalid sensor reading.
    //
    // A temporary ultrasonic failure should not accidentally remove an
    // already active safety/control condition.
    if (!snap.levelValid ||
        !isValidNumber(snap.levelPercent))
    {
      return;
    }

    bool trigger = false;
    bool normal = false;

    switch (tankLatchMode)
    {
    case LATCH_AT_FULL:

      // Trigger when tank reaches/exceeds full threshold.
      trigger =
          snap.levelPercent >= fullLevelPercent;

      // Reset when tank falls below full threshold.
      normal =
          snap.levelPercent < fullLevelPercent;

      break;

    case LATCH_AT_LOW:

      // Trigger when tank reaches/falls below low threshold.
      trigger =
          snap.levelPercent <= lowLevelPercent;

      // Reset when tank rises above low threshold.
      normal =
          snap.levelPercent > lowLevelPercent;

      break;

    case LATCH_OFF:
    default:

      tankLatchTriggered = false;
      return;
    }

    // -------------------------------------------------------------
    // Trigger latch
    // -------------------------------------------------------------

    if (trigger)
    {
      tankLatchTriggered = true;
    }

    // -------------------------------------------------------------
    // Automatic reset
    // -------------------------------------------------------------

    if (normal)
    {
      tankLatchTriggered = false;
    }
  }

  // =========================================================================
  // Temperature latch
  // =========================================================================

  static void updateTemperatureLatch()
  {
    // Automation disabled.
    if (temperatureLatchMode == TEMP_LATCH_OFF)
    {
      temperatureLatchTriggered = false;
      return;
    }

    // Do not reset a triggered latch because the temperature sensor
    // temporarily becomes invalid.
    if (!snap.tempValid ||
        !isValidNumber(snap.temperatureC))
    {
      return;
    }

    bool trigger = false;
    bool normal = false;

    switch (temperatureLatchMode)
    {
    case TEMP_LATCH_AT_LOW:

      // Temperature is below the low threshold.
      trigger =
          snap.temperatureC < lowTemperatureC;

      // Return to normal at or above the low threshold.
      normal =
          snap.temperatureC >= lowTemperatureC;

      break;

    case TEMP_LATCH_AT_HIGH:

      // Temperature is above the high threshold.
      trigger =
          snap.temperatureC > highTemperatureC;

      // Return to normal at or below the high threshold.
      normal =
          snap.temperatureC <= highTemperatureC;

      break;

    case TEMP_LATCH_OFF:
    default:

      temperatureLatchTriggered = false;
      return;
    }

    // -------------------------------------------------------------
    // Trigger latch
    // -------------------------------------------------------------

    if (trigger)
    {
      temperatureLatchTriggered = true;
    }

    // -------------------------------------------------------------
    // Automatic reset
    // -------------------------------------------------------------

    if (normal)
    {
      temperatureLatchTriggered = false;
    }
  }

  // =========================================================================
  // Combined relay control
  // =========================================================================

  static void updateAutomaticRelay()
  {
    const bool automaticDemand =
        tankLatchTriggered ||
        temperatureLatchTriggered;

    // =====================================================================
    // At least one automatic condition requires the relay
    // =====================================================================

    if (automaticDemand)
    {
      // Turn relay ON the first time an automatic latch demands it.
      if (!automaticRelayActive)
      {
        load_relay::turnOn();

        automaticRelayActive = true;
      }
      else
      {
        // Make sure the relay remains ON if another module or command
        // happened to turn it OFF while the automatic condition is
        // still active.
        if (!load_relay::isOn())
        {
          load_relay::turnOn();
        }
      }

      return;
    }

    // =====================================================================
    // No automatic condition remains
    // =====================================================================

    if (automaticRelayActive)
    {
      // This is the actual automatic RESET.
      //
      // The old implementation only cleared its latch variable.
      // It never commanded the physical relay OFF.
      load_relay::turnOff();

      automaticRelayActive = false;
    }
  }

  // =========================================================================
  // Begin
  // =========================================================================

  void begin()
  {
    memset(&snap, 0, sizeof(snap));

    snap.uptimeMs = 0;

    snap.temperatureC = NAN;
    snap.distanceCm = NAN;
    snap.levelPercent = NAN;

    snap.tempValid = false;
    snap.levelValid = false;

    snap.vibrationReady = false;

    snap.relayOn = load_relay::isOn();
    snap.relayRequested =
        load_relay::getRequestedState();

    snap.temperatureStatus =
        TEMP_STATUS_INVALID;

    snap.tankStatus =
        TANK_STATUS_INVALID;

    snap.tankLatchTriggered = false;
    snap.temperatureLatchTriggered = false;

    snap.automaticRelayDemand = false;

    snap.fullDistanceCm =
        fullDistanceCm;

    snap.lowDistanceCm =
        lowDistanceCm;

    snap.lowTemperatureC =
        lowTemperatureC;

    snap.highTemperatureC =
        highTemperatureC;

    snap.fullLevelPercent =
        fullLevelPercent;

    snap.lowLevelPercent =
        lowLevelPercent;

    snap.relayLatchMode =
        tankLatchMode;

    snap.temperatureLatchMode =
        temperatureLatchMode;

    tankLatchTriggered = false;
    temperatureLatchTriggered = false;
    automaticRelayActive = false;

    lastUpdate = 0;
  }

  // =========================================================================
  // Update
  // =========================================================================

  void update()
  {
    const unsigned long now = millis();

    if ((unsigned long)(now - lastUpdate) <
        UPDATE_INTERVAL_MS)
    {
      return;
    }

    lastUpdate = now;

    // ---------------------------------------------------------------------
    // Sensor readings
    // ---------------------------------------------------------------------

    snap.uptimeMs = now;

    snap.temperatureC =
        temp_sensor::getTemperatureC();

    snap.distanceCm =
        ultrasonic_sensor::getDistanceCm();

    snap.tempValid =
        temp_sensor::isValid();

    snap.levelValid =
        isValidNumber(snap.distanceCm) &&
        snap.distanceCm > 0.0f;

    // ---------------------------------------------------------------------
    // Tank level
    // ---------------------------------------------------------------------

    snap.levelPercent =
        calculateLevel(snap.distanceCm);

    if (!isValidNumber(snap.levelPercent))
    {
      snap.levelValid = false;
    }

    // ---------------------------------------------------------------------
    // Current status
    // ---------------------------------------------------------------------

    snap.temperatureStatus =
        calculateTemperatureStatus(
            snap.temperatureC);

    snap.tankStatus =
        calculateTankStatus(
            snap.levelPercent);

    // ---------------------------------------------------------------------
    // Vibration
    // ---------------------------------------------------------------------

    snap.vibrationReady = false;

    // ---------------------------------------------------------------------
    // Update independent latches
    // ---------------------------------------------------------------------

    updateTankLatch();

    updateTemperatureLatch();

    // ---------------------------------------------------------------------
    // Coordinate single physical relay
    // ---------------------------------------------------------------------

    updateAutomaticRelay();

    // ---------------------------------------------------------------------
    // Update snapshot
    // ---------------------------------------------------------------------

    snap.tankLatchTriggered =
        tankLatchTriggered;

    snap.temperatureLatchTriggered =
        temperatureLatchTriggered;

    snap.automaticRelayDemand =
        tankLatchTriggered ||
        temperatureLatchTriggered;

    snap.relayOn =
        load_relay::isOn();

    snap.relayRequested =
        load_relay::getRequestedState();

    snap.fullDistanceCm =
        fullDistanceCm;

    snap.lowDistanceCm =
        lowDistanceCm;

    snap.lowTemperatureC =
        lowTemperatureC;

    snap.highTemperatureC =
        highTemperatureC;

    snap.fullLevelPercent =
        fullLevelPercent;

    snap.lowLevelPercent =
        lowLevelPercent;

    snap.relayLatchMode =
        tankLatchMode;

    snap.temperatureLatchMode =
        temperatureLatchMode;
  }

  // =========================================================================
  // Snapshot
  // =========================================================================

  Snapshot getSnapshot()
  {
    return snap;
  }

  // =========================================================================
  // Tank level
  // =========================================================================

  float getLevelPercent()
  {
    return snap.levelPercent;
  }

  // =========================================================================
  // Legacy percentage thresholds
  // =========================================================================

  void setLevelThresholds(float fullPercent,
                          float lowPercent)
  {
    if (!isValidNumber(fullPercent) ||
        !isValidNumber(lowPercent))
    {
      return;
    }

    if (fullPercent <= lowPercent)
      return;

    if (fullPercent > 100.0f)
      return;

    if (lowPercent < 0.0f)
      return;

    fullLevelPercent =
        fullPercent;

    lowLevelPercent =
        lowPercent;

    // Re-evaluate latch state against the new thresholds.
    tankLatchTriggered = false;
  }

  float getFullLevelPercent()
  {
    return fullLevelPercent;
  }

  float getLowLevelPercent()
  {
    return lowLevelPercent;
  }

  // =========================================================================
  // Tank distance calibration
  // =========================================================================

  bool setTankDistances(float fullDistance,
                        float lowDistance)
  {
    if (!isValidNumber(fullDistance) ||
        !isValidNumber(lowDistance))
    {
      return false;
    }

    if (fullDistance <= 0.0f)
      return false;

    // For a top-mounted ultrasonic sensor:
    //
    // FULL distance must be smaller than LOW/EMPTY distance.
    //
    if (lowDistance <= fullDistance)
      return false;

    fullDistanceCm =
        fullDistance;

    lowDistanceCm =
        lowDistance;

    // New calibration means the previous tank latch state should be
    // evaluated using the new calibration.
    tankLatchTriggered = false;

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

  // =========================================================================
  // Temperature thresholds
  // =========================================================================

  bool setTemperatureThresholds(
      float lowTemperature,
      float highTemperature)
  {
    if (!isValidNumber(lowTemperature) ||
        !isValidNumber(highTemperature))
    {
      return false;
    }

    if (highTemperature <= lowTemperature)
      return false;

    lowTemperatureC =
        lowTemperature;

    highTemperatureC =
        highTemperature;

    // Configuration changed, therefore clear the old latch state.
    temperatureLatchTriggered = false;

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

  // =========================================================================
  // Temperature status
  // =========================================================================

  TemperatureStatus getTemperatureStatus()
  {
    return snap.temperatureStatus;
  }

  const char *getTemperatureStatusText()
  {
    switch (snap.temperatureStatus)
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

  // =========================================================================
  // Tank relay latch
  // =========================================================================

  void setRelayLatchMode(RelayLatchMode mode)
  {
    if (mode > LATCH_AT_LOW)
      return;

    tankLatchMode = mode;

    // Changing automation configuration resets the existing latch.
    tankLatchTriggered = false;

    // If tank automation was responsible for the relay and no other
    // automatic source remains, updateAutomaticRelay() will turn it off
    // during the next update cycle.
  }

  RelayLatchMode getRelayLatchMode()
  {
    return tankLatchMode;
  }

  bool isTankLatchTriggered()
  {
    return tankLatchTriggered;
  }

  // =========================================================================
  // Temperature relay latch
  // =========================================================================

  void setTemperatureLatchMode(
      TemperatureLatchMode mode)
  {
    if (mode > TEMP_LATCH_AT_HIGH)
      return;

    temperatureLatchMode =
        mode;

    // Changing the mode resets its previous latch.
    temperatureLatchTriggered = false;
  }

  TemperatureLatchMode getTemperatureLatchMode()
  {
    return temperatureLatchMode;
  }

  bool isTemperatureLatchTriggered()
  {
    return temperatureLatchTriggered;
  }

  // =========================================================================
  // Compatibility aliases
  // =========================================================================

  void setTemperatureRelayLatchMode(
      TemperatureLatchMode mode)
  {
    setTemperatureLatchMode(mode);
  }

  TemperatureLatchMode getTemperatureRelayLatchMode()
  {
    return getTemperatureLatchMode();
  }

  // =========================================================================
  // Combined automatic relay demand
  // =========================================================================

  bool isAutomaticRelayDemand()
  {
    return tankLatchTriggered ||
           temperatureLatchTriggered;
  }

  // =========================================================================
  // Apply all settings
  // =========================================================================

  bool applySettings(
      float fullDistance,
      float lowDistance,
      float lowTemperature,
      float highTemperature,
      RelayLatchMode newTankLatchMode,
      TemperatureLatchMode newTemperatureLatchMode)
  {
    // Validate all settings BEFORE modifying anything.
    if (!isValidNumber(fullDistance) ||
        !isValidNumber(lowDistance) ||
        !isValidNumber(lowTemperature) ||
        !isValidNumber(highTemperature))
    {
      return false;
    }

    if (fullDistance <= 0.0f)
      return false;

    if (lowDistance <= fullDistance)
      return false;

    if (highTemperature <= lowTemperature)
      return false;

    if (newTankLatchMode > LATCH_AT_LOW)
      return false;

    if (newTemperatureLatchMode > TEMP_LATCH_AT_HIGH)
      return false;

    // ---------------------------------------------------------------------
    // Apply settings
    // ---------------------------------------------------------------------

    fullDistanceCm =
        fullDistance;

    lowDistanceCm =
        lowDistance;

    lowTemperatureC =
        lowTemperature;

    highTemperatureC =
        highTemperature;

    tankLatchMode =
        newTankLatchMode;

    temperatureLatchMode =
        newTemperatureLatchMode;

    // ---------------------------------------------------------------------
    // Reset previous latch states.
    //
    // The next update cycle evaluates the actual sensors against the new
    // configuration.
    // ---------------------------------------------------------------------

    tankLatchTriggered = false;

    temperatureLatchTriggered = false;

    return true;
  }

  // =========================================================================
  // Clear automatic latches
  // =========================================================================

  void clearAutomaticLatches()
  {
    tankLatchTriggered = false;

    temperatureLatchTriggered = false;

    if (automaticRelayActive)
    {
      load_relay::turnOff();

      automaticRelayActive = false;
    }
  }
}
