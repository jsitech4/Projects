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

// =========================================================
// SERIAL REPORT
// =========================================================

static unsigned long lastSerialReport = 0;

static constexpr unsigned long
    SERIAL_REPORT_INTERVAL_MS = 3000;

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
  Serial.println(
      "========== HOT LIQUID SYSTEM ==========");

  // -------------------------------------------------------
  // Temperature
  // -------------------------------------------------------

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

  Serial.print("Temperature status: ");
  Serial.println(
      device_manager::
          getTemperatureStatusText());

  // -------------------------------------------------------
  // Ultrasonic / Tank
  // -------------------------------------------------------

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

  // -------------------------------------------------------
  // Relay
  // -------------------------------------------------------

  Serial.print("Relay: ");
  Serial.println(
      snap.relayOn
          ? "ON"
          : "OFF");

  // -------------------------------------------------------
  // Tank latch
  // -------------------------------------------------------

  Serial.print("Tank latch: ");
  Serial.println(
      snap.tankLatchTriggered
          ? "TRIGGERED"
          : "NOT TRIGGERED");

  // -------------------------------------------------------
  // Temperature latch
  // -------------------------------------------------------

  Serial.print("Temperature latch: ");
  Serial.println(
      snap.temperatureLatchTriggered
          ? "TRIGGERED"
          : "NOT TRIGGERED");

  // -------------------------------------------------------
  // Automatic relay demand
  // -------------------------------------------------------

  Serial.print("Automatic relay demand: ");
  Serial.println(
      snap.automaticRelayDemand
          ? "YES"
          : "NO");

  Serial.println(
      "========================================");
}

// =========================================================
// SETUP
// =========================================================

void setup()
{
  delay(300);

  Serial.begin(115200);

  delay(100);

  Serial.println();
  Serial.println(
      "Starting Hot Liquid Monitoring and Control System...");

  // -------------------------------------------------------
  // Hardware
  // -------------------------------------------------------

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

  // -------------------------------------------------------
  // Device manager
  // -------------------------------------------------------

  device_manager::begin();

  // -------------------------------------------------------
  // Storage
  // -------------------------------------------------------

  storage::begin();

  // -------------------------------------------------------
  // Load persistent settings
  // -------------------------------------------------------

  float fullDistance =
      device_manager::
          getFullDistanceCm();

  float lowDistance =
      device_manager::
          getLowDistanceCm();

  float lowTemperature =
      device_manager::
          getLowTemperatureC();

  float highTemperature =
      device_manager::
          getHighTemperatureC();

  uint8_t tankLatch =
      static_cast<uint8_t>(
          device_manager::
              getRelayLatchMode());

  uint8_t temperatureLatch =
      static_cast<uint8_t>(
          device_manager::
              getTemperatureLatchMode());

  // -------------------------------------------------------
  // Load settings from storage
  // -------------------------------------------------------

  bool settingsLoaded =
      storage::loadTankSettings(
          fullDistance,
          lowDistance,
          tankLatch,
          lowTemperature,
          highTemperature,
          temperatureLatch);

  // -------------------------------------------------------
  // Apply saved settings
  // -------------------------------------------------------

  if (settingsLoaded)
  {
    bool applied =
        device_manager::applySettings(
            fullDistance,
            lowDistance,
            lowTemperature,
            highTemperature,

            static_cast<
                device_manager::
                    RelayLatchMode>(
                tankLatch),

            static_cast<
                device_manager::
                    TemperatureLatchMode>(
                temperatureLatch));

    if (applied)
    {
      storage::logEvent(
          "SETTINGS",
          "Saved settings restored from internal flash.");

      Serial.println(
          "Saved settings restored.");
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

  // -------------------------------------------------------
  // Web dashboard
  // -------------------------------------------------------

  local_server::begin();

  // -------------------------------------------------------
  // LCD
  // -------------------------------------------------------

  lcd_screen::begin();

  // -------------------------------------------------------
  // Startup log
  // -------------------------------------------------------

  storage::logEvent(
      "BOOT",
      "Hot Liquid Monitoring and Control System started.");

  buzzer::beep(120);

  // -------------------------------------------------------
  // Serial startup information
  // -------------------------------------------------------

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

// =========================================================
// LOOP
// =========================================================

void loop()
{
  // -------------------------------------------------------
  // Sensors
  // -------------------------------------------------------

  temp_sensor::update();

  ultrasonic_sensor::update();

  // -------------------------------------------------------
  // System control
  // -------------------------------------------------------

  device_manager::update();

  load_relay::update();

  // -------------------------------------------------------
  // User interface / indicators
  // -------------------------------------------------------

  buzzer::update();

  led_indicator::update();

  lcd_screen::update();

  // -------------------------------------------------------
  // Storage
  // -------------------------------------------------------

  storage::update();

  // -------------------------------------------------------
  // Dashboard
  // -------------------------------------------------------

  local_server::update();

  // -------------------------------------------------------
  // Power / reset
  // -------------------------------------------------------

  sleep_wake::update();

  reset::update();

  // -------------------------------------------------------
  // Diagnostics
  // -------------------------------------------------------

  printSerialReport();

  yield();
}
