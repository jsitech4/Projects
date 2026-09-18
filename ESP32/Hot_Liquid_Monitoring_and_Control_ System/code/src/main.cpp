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

  Serial.println();
  Serial.println("========== HOT LIQUID SYSTEM ==========");

  // ----------------------------------------------------------
  // Temperature
  // ----------------------------------------------------------

  if (snap.tempValid)
  {
    Serial.print("Temperature: ");
    Serial.print(
        snap.temperatureC,
        2);
    Serial.println(" C");
  }
  else
  {
    Serial.println(
        "Temperature: INVALID");
  }

  Serial.print(
      "Temperature status: ");

  Serial.println(
      device_manager::getTemperatureStatusText());

  // ----------------------------------------------------------
  // Ultrasonic / Level
  // ----------------------------------------------------------

  if (snap.levelValid)
  {
    Serial.print("Distance: ");
    Serial.print(
        snap.distanceCm,
        2);
    Serial.println(" cm");

    Serial.print("Level: ");
    Serial.print(
        snap.levelPercent,
        1);
    Serial.println(" %");
  }
  else
  {
    Serial.println(
        "Ultrasonic: INVALID");
  }

  // ----------------------------------------------------------
  // Relay
  // ----------------------------------------------------------

  Serial.print("Relay physical command: ");

  Serial.println(
      snap.relayOn
          ? "ON"
          : "OFF");

  Serial.print(
      "Manual relay request: ");

  Serial.println(
      snap.manualRelayRequest
          ? "ON"
          : "OFF");

  Serial.print(
      "Automatic relay demand: ");

  Serial.println(
      snap.automaticRelayDemand
          ? "YES"
          : "NO");

  Serial.print(
      "Final relay request: ");

  Serial.println(
      snap.relayRequested
          ? "ON"
          : "OFF");

  // ----------------------------------------------------------
  // Tank latch
  // ----------------------------------------------------------

  Serial.print(
      "Tank latch: ");

  Serial.println(
      snap.tankLatchTriggered
          ? "TRIGGERED"
          : "NOT TRIGGERED");

  // ----------------------------------------------------------
  // Temperature latch
  // ----------------------------------------------------------

  Serial.print(
      "Temperature latch: ");

  Serial.println(
      snap.temperatureLatchTriggered
          ? "TRIGGERED"
          : "NOT TRIGGERED");

  // ----------------------------------------------------------
  // Thresholds
  // ----------------------------------------------------------

  Serial.print(
      "Full distance: ");

  Serial.print(
      snap.fullDistanceCm,
      2);

  Serial.println(" cm");

  Serial.print(
      "Low distance: ");

  Serial.print(
      snap.lowDistanceCm,
      2);

  Serial.println(" cm");

  Serial.print(
      "Full level threshold: ");

  Serial.print(
      snap.fullLevelPercent,
      1);

  Serial.println(" %");

  Serial.print(
      "Low level threshold: ");

  Serial.print(
      snap.lowLevelPercent,
      1);

  Serial.println(" %");

  Serial.print(
      "Low temperature threshold: ");

  Serial.print(
      snap.lowTemperatureC,
      2);

  Serial.println(" C");

  Serial.print(
      "High temperature threshold: ");

  Serial.print(
      snap.highTemperatureC,
      2);

  Serial.println(" C");

  Serial.println(
      "========================================");
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  delay(300);

  Serial.begin(115200);

  delay(100);

  Serial.println();

  Serial.println(
      "Starting Hot Liquid Monitoring and Control System...");

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

      Serial.println(
          "Saved settings restored.");

      Serial.print(
          "Full distance: ");

      Serial.print(
          fullDistance,
          2);

      Serial.println(" cm");

      Serial.print(
          "Low distance: ");

      Serial.print(
          lowDistance,
          2);

      Serial.println(" cm");

      Serial.print(
          "Full level: ");

      Serial.print(
          fullLevel,
          1);

      Serial.println(" %");

      Serial.print(
          "Low level: ");

      Serial.print(
          lowLevel,
          1);

      Serial.println(" %");

      Serial.print(
          "Low temperature: ");

      Serial.print(
          lowTemperature,
          2);

      Serial.println(" C");

      Serial.print(
          "High temperature: ");

      Serial.print(
          highTemperature,
          2);

      Serial.println(" C");
    }
    else
    {
      storage::logEvent(
          "SETTINGS",
          "Saved settings were invalid and defaults were retained.");

      Serial.println(
          "Saved settings invalid; defaults retained.");
    }
  }
  else
  {
    storage::logEvent(
        "SETTINGS",
        "No valid saved settings. Default settings are active.");

    Serial.println(
        "No valid saved settings. Defaults active.");
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
      "Hot Liquid Monitoring and Control System started.");

  buzzer::beep(120);

  // ==========================================================
  // READY
  // ==========================================================

  Serial.println();

  Serial.println(
      "System ready.");

  Serial.print(
      "Dashboard SSID: ");

  Serial.println(
      local_server::getSsid());

  Serial.print(
      "Dashboard IP: http://");

  Serial.println(
      local_server::getIp());

  Serial.println();
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

  printSerialReport();

  yield();
}
