#include <Arduino.h>
#include <math.h>

#include "device_manager.h"

#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "load_relay/load_relay.h"

namespace device_manager
{
  // =========================================================
  // DEFAULT SETTINGS
  // =========================================================

  static constexpr float DEFAULT_FULL_DISTANCE_CM = 2.0f;
  static constexpr float DEFAULT_LOW_DISTANCE_CM = 40.0f;

  static constexpr float DEFAULT_FULL_LEVEL_PERCENT = 90.0f;
  static constexpr float DEFAULT_LOW_LEVEL_PERCENT = 10.0f;

  static constexpr float DEFAULT_LOW_TEMPERATURE_C = 20.0f;
  static constexpr float DEFAULT_HIGH_TEMPERATURE_C = 80.0f;

  static constexpr RelayLatchMode DEFAULT_TANK_LATCH_MODE =
      LATCH_OFF;

  static constexpr TemperatureLatchMode DEFAULT_TEMPERATURE_LATCH_MODE =
      TEMP_LATCH_OFF;

  // =========================================================
  // RUNTIME SETTINGS
  // =========================================================

  static float fullDistanceCm =
      DEFAULT_FULL_DISTANCE_CM;

  static float lowDistanceCm =
      DEFAULT_LOW_DISTANCE_CM;

  static float fullLevelPercent =
      DEFAULT_FULL_LEVEL_PERCENT;

  static float lowLevelPercent =
      DEFAULT_LOW_LEVEL_PERCENT;

  static RelayLatchMode relayLatchMode =
      DEFAULT_TANK_LATCH_MODE;

  static float lowTemperatureC =
      DEFAULT_LOW_TEMPERATURE_C;

  static float highTemperatureC =
      DEFAULT_HIGH_TEMPERATURE_C;

  static TemperatureLatchMode temperatureLatchMode =
      DEFAULT_TEMPERATURE_LATCH_MODE;

  // =========================================================
  // LATCH STATE
  // =========================================================

  static bool tankLatchTriggered = false;
  static bool temperatureLatchTriggered = false;

  // =========================================================
  // SNAPSHOT
  // =========================================================

  static Snapshot snap;

  static unsigned long lastUpdate = 0;

  static constexpr unsigned long UPDATE_INTERVAL_MS = 250;

  // =========================================================
  // HELPERS
  // =========================================================

  static float clampFloat(float value,
                          float low,
                          float high)
  {
    if (value < low)
      return low;

    if (value > high)
      return high;

    return value;
  }

  static float calculateLevel(float distanceCm)
  {
    if (!isfinite(distanceCm) ||
        distanceCm <= 0.0f)
    {
      return NAN;
    }

    if (lowDistanceCm <= fullDistanceCm)
    {
      return NAN;
    }

    /*
     * Example:
     *
     * fullDistance = 2 cm
     * lowDistance  = 40 cm
     *
     * 2 cm  -> 100 %
     * 40 cm ->   0 %
     */

    float level =
        ((lowDistanceCm - distanceCm) /
         (lowDistanceCm - fullDistanceCm)) *
        100.0f;

    return clampFloat(level, 0.0f, 100.0f);
  }

  static TemperatureStatus calculateTemperatureStatus(
      float temperatureC)
  {
    if (!isfinite(temperatureC))
    {
      return TEMP_STATUS_INVALID;
    }

    if (temperatureC < lowTemperatureC)
    {
      return TEMP_STATUS_LOW;
    }

    if (temperatureC > highTemperatureC)
    {
      return TEMP_STATUS_HIGH;
    }

    return TEMP_STATUS_NORMAL;
  }

  // =========================================================
  // BEGIN
  // =========================================================

  void begin()
  {
    memset(&snap, 0, sizeof(snap));

    snap.temperatureC = NAN;
    snap.distanceCm = NAN;
    snap.levelPercent = NAN;

    snap.tempValid = false;
    snap.levelValid = false;
    snap.vibrationReady = false;

    snap.relayOn = false;
    snap.relayRequested = false;

    snap.tankLatchTriggered = false;
    snap.temperatureLatchTriggered = false;

    tankLatchTriggered = false;
    temperatureLatchTriggered = false;

    lastUpdate = 0;
  }

  // =========================================================
  // UPDATE
  // =========================================================

  void update()
  {
    unsigned long now = millis();

    if ((unsigned long)(now - lastUpdate) <
        UPDATE_INTERVAL_MS)
    {
      return;
    }

    lastUpdate = now;

    // -----------------------------------------------------
    // Sensor readings
    // -----------------------------------------------------

    snap.uptimeMs = now;

    snap.temperatureC =
        temp_sensor::getTemperatureC();

    snap.distanceCm =
        ultrasonic_sensor::getDistanceCm();

    snap.tempValid =
        temp_sensor::isValid();

    snap.levelValid =
        isfinite(snap.distanceCm) &&
        snap.distanceCm > 0.0f;

    if (snap.levelValid)
    {
      snap.levelPercent =
          calculateLevel(snap.distanceCm);
    }
    else
    {
      snap.levelPercent = NAN;
    }

    snap.vibrationReady = false;

    // -----------------------------------------------------
    // Temperature status
    // -----------------------------------------------------

    TemperatureStatus temperatureStatus =
        calculateTemperatureStatus(
            snap.temperatureC);

    // -----------------------------------------------------
    // Tank latch condition
    // -----------------------------------------------------

    bool tankConditionReached = false;

    if (snap.levelValid)
    {
      if (relayLatchMode == LATCH_AT_FULL)
      {
        tankConditionReached =
            snap.levelPercent >=
            fullLevelPercent;
      }
      else if (relayLatchMode == LATCH_AT_LOW)
      {
        tankConditionReached =
            snap.levelPercent <=
            lowLevelPercent;
      }
    }

    // -----------------------------------------------------
    // Temperature latch condition
    // -----------------------------------------------------

    bool temperatureConditionReached = false;

    if (snap.tempValid)
    {
      if (temperatureLatchMode ==
          TEMP_LATCH_LOW)
      {
        temperatureConditionReached =
            temperatureStatus ==
            TEMP_STATUS_LOW;
      }
      else if (temperatureLatchMode ==
               TEMP_LATCH_HIGH)
      {
        temperatureConditionReached =
            temperatureStatus ==
            TEMP_STATUS_HIGH;
      }
    }

    // -----------------------------------------------------
    // Reset trigger availability after condition clears
    //
    // The relay itself is NOT automatically turned off.
    // This preserves the existing latch behavior.
    // -----------------------------------------------------

    if (!tankConditionReached)
    {
      tankLatchTriggered = false;
    }

    if (!temperatureConditionReached)
    {
      temperatureLatchTriggered = false;
    }

    // -----------------------------------------------------
    // Trigger tank latch
    // -----------------------------------------------------

    if (tankConditionReached &&
        !tankLatchTriggered)
    {
      tankLatchTriggered = true;

      load_relay::turnOn();
    }

    // -----------------------------------------------------
    // Trigger temperature latch
    // -----------------------------------------------------

    if (temperatureConditionReached &&
        !temperatureLatchTriggered)
    {
      temperatureLatchTriggered = true;

      load_relay::turnOn();
    }

    // -----------------------------------------------------
    // Final relay state
    // -----------------------------------------------------

    snap.relayOn =
        load_relay::isOn();

    snap.relayRequested =
        load_relay::getRequestedState();

    snap.tankLatchTriggered =
        tankLatchTriggered;

    snap.temperatureLatchTriggered =
        temperatureLatchTriggered;
  }

  // =========================================================
  // SNAPSHOT
  // =========================================================

  Snapshot getSnapshot()
  {
    return snap;
  }

  // =========================================================
  // LEVEL
  // =========================================================

  float getLevelPercent()
  {
    return snap.levelPercent;
  }

  float getFullDistanceCm()
  {
    return fullDistanceCm;
  }

  float getLowDistanceCm()
  {
    return lowDistanceCm;
  }

  bool setTankDistances(float fullDistance,
                        float lowDistance)
  {
    if (!isfinite(fullDistance) ||
        !isfinite(lowDistance))
    {
      return false;
    }

    if (fullDistance <= 0.0f)
    {
      return false;
    }

    if (lowDistance <= fullDistance)
    {
      return false;
    }

    if (lowDistance > 500.0f)
    {
      return false;
    }

    fullDistanceCm = fullDistance;
    lowDistanceCm = lowDistance;

    return true;
  }

  // =========================================================
  // LEVEL THRESHOLDS
  // =========================================================

  float getFullLevelPercent()
  {
    return fullLevelPercent;
  }

  float getLowLevelPercent()
  {
    return lowLevelPercent;
  }

  bool setLevelThresholds(float fullPercent,
                          float lowPercent)
  {
    if (!isfinite(fullPercent) ||
        !isfinite(lowPercent))
    {
      return false;
    }

    if (fullPercent <= lowPercent)
    {
      return false;
    }

    if (fullPercent > 100.0f)
    {
      return false;
    }

    if (lowPercent < 0.0f)
    {
      return false;
    }

    fullLevelPercent = fullPercent;
    lowLevelPercent = lowPercent;

    return true;
  }

  // =========================================================
  // TANK LATCH
  // =========================================================

  void setRelayLatchMode(RelayLatchMode mode)
  {
    if (mode > LATCH_AT_LOW)
    {
      return;
    }

    relayLatchMode = mode;

    /*
     * Changing the mode makes the new mode start cleanly.
     */
    tankLatchTriggered = false;
  }

  RelayLatchMode getRelayLatchMode()
  {
    return relayLatchMode;
  }

  // =========================================================
  // TEMPERATURE
  // =========================================================

  float getLowTemperatureC()
  {
    return lowTemperatureC;
  }

  float getHighTemperatureC()
  {
    return highTemperatureC;
  }

  bool setTemperatureThresholds(float lowTemperature,
                                float highTemperature)
  {
    if (!isfinite(lowTemperature) ||
        !isfinite(highTemperature))
    {
      return false;
    }

    if (lowTemperature >= highTemperature)
    {
      return false;
    }

    if (lowTemperature < -200.0f ||
        highTemperature > 850.0f)
    {
      return false;
    }

    lowTemperatureC = lowTemperature;
    highTemperatureC = highTemperature;

    return true;
  }

  TemperatureStatus getTemperatureStatus()
  {
    return calculateTemperatureStatus(
        snap.temperatureC);
  }

  const char *getTemperatureStatusText()
  {
    switch (getTemperatureStatus())
    {
    case TEMP_STATUS_LOW:
      return "LOW";

    case TEMP_STATUS_NORMAL:
      return "NORMAL";

    case TEMP_STATUS_HIGH:
      return "HIGH";

    default:
      return "INVALID";
    }
  }

  // =========================================================
  // TEMPERATURE LATCH
  // =========================================================

  void setTemperatureLatchMode(
      TemperatureLatchMode mode)
  {
    if (mode > TEMP_LATCH_HIGH)
    {
      return;
    }

    temperatureLatchMode = mode;

    temperatureLatchTriggered = false;
  }

  TemperatureLatchMode getTemperatureLatchMode()
  {
    return temperatureLatchMode;
  }

  // =========================================================
  // APPLY ALL SETTINGS
  // =========================================================

  bool applySettings(
      float fullDistance,
      float lowDistance,
      float fullPercent,
      float lowPercent,
      RelayLatchMode tankMode,
      float lowTemperature,
      float highTemperature,
      TemperatureLatchMode temperatureMode)
  {
    if (!isfinite(fullDistance) ||
        !isfinite(lowDistance) ||
        !isfinite(fullPercent) ||
        !isfinite(lowPercent) ||
        !isfinite(lowTemperature) ||
        !isfinite(highTemperature))
    {
      return false;
    }

    if (fullDistance <= 0.0f ||
        lowDistance <= fullDistance ||
        lowDistance > 500.0f)
    {
      return false;
    }

    if (fullPercent <= lowPercent ||
        fullPercent > 100.0f ||
        lowPercent < 0.0f)
    {
      return false;
    }

    if (lowTemperature >= highTemperature)
    {
      return false;
    }

    if (lowTemperature < -200.0f ||
        highTemperature > 850.0f)
    {
      return false;
    }

    if (tankMode > LATCH_AT_LOW)
    {
      return false;
    }

    if (temperatureMode > TEMP_LATCH_HIGH)
    {
      return false;
    }

    fullDistanceCm = fullDistance;
    lowDistanceCm = lowDistance;

    fullLevelPercent = fullPercent;
    lowLevelPercent = lowPercent;

    relayLatchMode = tankMode;

    lowTemperatureC = lowTemperature;
    highTemperatureC = highTemperature;

    temperatureLatchMode = temperatureMode;

    tankLatchTriggered = false;
    temperatureLatchTriggered = false;

    return true;
  }
}
