#include <Arduino.h>
#include <Wire.h>

#include "Pins.h"

#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "buzzer/buzzer.h"
#include "load_relay/load_relay.h"
#include "lcd_screen/lcd_screen.h"
#include "led_indicator/led_indicator.h"
#include "storage/storage.h"
#include "sleep_wake/sleep_wake.h"
#include "reset/reset.h"
#include "device_manager/device_manager.h"
#include "local_server/local_server.h"

// ============================================================
// SERIAL REPORT
// ============================================================

static unsigned long lastSerialReport = 0;

static constexpr unsigned long SERIAL_REPORT_INTERVAL_MS =
    3000;

static void printSerialReport()
{
  unsigned long now = millis();

  if ((unsigned long)(now - lastSerialReport) <
      SERIAL_REPORT_INTERVAL_MS)
  {
    return;
  }

  lastSerialReport = now;

  device_manager::Snapshot snap =
      device_manager::getSnapshot();

  if (snap.tempValid)
  {
  }
  else
  {
  }

  if (snap.levelValid)
  {
  }
  else
  {
  }
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  delay(300);

  Serial.begin(115200);

  delay(100);

  // ==========================================================
  // HARDWARE INITIALIZATION
  // ==========================================================

  Pins::begin();

  Wire.begin(
      Pins::I2C_SDA,
      Pins::I2C_SCL);

  sleep_wake::begin();

  reset::begin();

  buzzer::begin();

  load_relay::begin();

  led_indicator::begin();

  temp_sensor::begin();

  ultrasonic_sensor::begin(
      Pins::ULTRASONIC_TRIG,
      Pins::ULTRASONIC_ECHO);

  // ==========================================================
  // DEVICE MANAGER
  // ==========================================================

  device_manager::begin();

  // ==========================================================
  // STORAGE
  // ==========================================================

  storage::begin();

  // ==========================================================
  // LOAD DEFAULT SETTINGS
  //
  // These are already supplied by device_manager.
  // We copy them into local variables so saved settings
  // can replace them if valid.
  // ==========================================================

  float fullDistance =
      device_manager::getFullDistanceCm();

  float lowDistance =
      device_manager::getLowDistanceCm();

  float fullLevel =
      device_manager::getFullLevelPercent();

  float lowLevel =
      device_manager::getLowLevelPercent();

  float lowTemperature =
      device_manager::getLowTemperatureC();

  float highTemperature =
      device_manager::getHighTemperatureC();

  uint8_t tankLatch =
      static_cast<uint8_t>(
          device_manager::getRelayLatchMode());

  uint8_t temperatureLatch =
      static_cast<uint8_t>(
          device_manager::getTemperatureLatchMode());

  // ==========================================================
  // LOAD SAVED SETTINGS
  // ==========================================================

  bool settingsLoaded =
      storage::loadTankSettings(
          fullDistance,
          lowDistance,
          fullLevel,
          lowLevel,
          tankLatch,
          lowTemperature,
          highTemperature,
          temperatureLatch);

  // ==========================================================
  // APPLY SAVED SETTINGS
  // ==========================================================

  if (settingsLoaded)
  {
    bool applied =
        device_manager::applySettings(
            fullDistance,
            lowDistance,
            lowTemperature,
            highTemperature,
            static_cast<
                device_manager::RelayLatchMode>(
                tankLatch),
            static_cast<
                device_manager::TemperatureLatchMode>(
                temperatureLatch));

    if (applied)
    {
      // ------------------------------------------------------
      // Level thresholds are managed separately because
      // applySettings() handles distance/temperature/latches.
      // ------------------------------------------------------

      device_manager::setLevelThresholds(
          fullLevel,
          lowLevel);

      storage::logEvent(
          "SETTINGS",
          "Saved settings restored from internal flash.");
    }
    else
    {
      storage::logEvent(
          "SETTINGS",
          "Saved settings were invalid and defaults were retained.");
    }
  }
  else
  {
    storage::logEvent(
        "SETTINGS",
        "No valid saved settings. Default settings are active.");
  }

  // ==========================================================
  // LOCAL WEB SERVER
  // ==========================================================

  local_server::begin();

  // ==========================================================
  // LCD
  // ==========================================================

  lcd_screen::begin();

  // ==========================================================
  // BOOT EVENT
  // ==========================================================

  storage::logEvent(
      "BOOT",
      "Water Level and Temperture Monitoring System started.");

  buzzer::beep(120);

  // ==========================================================
  // READY
  // ==========================================================
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ==========================================================
  // SENSOR UPDATES
  // ==========================================================

  temp_sensor::update();

  ultrasonic_sensor::update();

  // ==========================================================
  // DEVICE CONTROL
  //
  // device_manager owns the control decision.
  // ==========================================================

  device_manager::update();

  // ==========================================================
  // RELAY
  //
  // load_relay only executes the command supplied by the
  // device manager. It does not make control decisions.
  // ==========================================================

  load_relay::update();

  // ==========================================================
  // USER INTERFACE / INDICATORS
  // ==========================================================

  buzzer::update();

  led_indicator::update();

  lcd_screen::update();

  // ==========================================================
  // STORAGE / LOGGING
  // ==========================================================

  storage::update();

  // ==========================================================
  // WEB DASHBOARD
  // ==========================================================

  local_server::update();

  // ==========================================================
  // POWER / SYSTEM SERVICES
  // ==========================================================

  sleep_wake::update();

  reset::update();

  // ==========================================================
  // SERIAL MONITOR
  // ==========================================================

  // printSerialReport();

  yield();
}
