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
#include "maintenance_manager/maintenance_manager.h"
#include "local_server/local_server.h"

static unsigned long lastSerialReport = 0;
static const unsigned long SERIAL_REPORT_INTERVAL_MS = 3000;

static void printSerialReport()
{
  unsigned long now = millis();

  if (now - lastSerialReport < SERIAL_REPORT_INTERVAL_MS)
  {
    return;
  }

  lastSerialReport = now;

  maintenance_manager::Snapshot snap = maintenance_manager::getSnapshot();
}

void setup()
{
  delay(300);

  Pins::begin();
  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

  sleep_wake::begin();
  reset::begin();
  buzzer::begin();
  load_relay::begin();
  led_indicator::begin();

  temp_sensor::begin();
  ultrasonic_sensor::begin(Pins::ULTRASONIC_TRIG, Pins::ULTRASONIC_ECHO);

  maintenance_manager::begin();
  storage::begin();
  float fullLevel = maintenance_manager::getFullLevelPercent();
  float lowLevel = maintenance_manager::getLowLevelPercent();
  uint8_t latchMode = maintenance_manager::getRelayLatchMode();
  if (storage::loadTankSettings(fullLevel, lowLevel, latchMode))
  {
    maintenance_manager::setLevelThresholds(fullLevel, lowLevel);
    maintenance_manager::setRelayLatchMode(static_cast<maintenance_manager::RelayLatchMode>(latchMode));
  }
  local_server::begin();
  lcd_screen::begin();

  storage::logEvent("BOOT", "Predictive maintenance firmware started.");
  buzzer::beep(120);
}

void loop()
{
  temp_sensor::update();
  ultrasonic_sensor::update();
  maintenance_manager::update();
  load_relay::update();

  buzzer::update();
  led_indicator::update();
  lcd_screen::update();

  storage::update();
  local_server::update();

  sleep_wake::update();
  reset::update();

  yield();
}
