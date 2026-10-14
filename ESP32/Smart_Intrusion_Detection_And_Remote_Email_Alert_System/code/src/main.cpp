#include <Arduino.h>
#include "Sensors/pir_sensor/pir_sensor.h"
#include "Actuators/buzzer/buzzer.h"
#include "Power/battery_level/battery_level.h"
#include "Storage/config/config.h"
#include "Control/intrusion_controller/intrusion_controller.h"
#include "Communication/wifi_manager/wifi_manager.h"
#include "Communication/email_manager/email_manager.h"
#include "Storage/event_log/event_log.h"
#include "Web/web_admin/web_admin.h"
#include "Control/error_handling/error_handling.h"
#include "Control/status_indicator_logic/status_indicator_logic.h"
#include "Display/led_indicator/led_indicator.h"
#include "Input/button/button.h"

// The repository stores component implementations in nested folders rather
// than PlatformIO library roots, so they are compiled together here.
#include "../lib/Pins/Pins.cpp"
#include "../lib/Sensors/pir_sensor/pir_sensor.cpp"
#include "../lib/Actuators/buzzer/buzzer.cpp"
#include "../lib/Power/battery_level/battery_level.cpp"
#include "../lib/Control/error_handling/error_handling.cpp"
#include "../lib/Storage/config/config.cpp"
#include "../lib/Control/intrusion_controller/intrusion_controller.cpp"
#include "../lib/Communication/wifi_manager/wifi_manager.cpp"
#include "../lib/Communication/email_manager/email_manager.cpp"
#include "../lib/Storage/event_log/event_log.cpp"
#include "../lib/Web/web_admin/web_admin.cpp"
#include "../lib/Display/led_indicator/led_indicator.cpp"
#include "../lib/Input/button/button.cpp"

// Verified from the original PCB firmware. Do not substitute generic ESP32 pins.
static constexpr uint8_t PIR_PIN = 26;
static constexpr uint8_t BUZZER_PIN = 23;
static constexpr uint8_t BATTERY_PIN = 34;   // ADC input, original PCB divider scale is 1.61
static constexpr uint8_t STATUS_LED_PIN = 2; // verified original-board status LED, active-HIGH
static constexpr uint8_t BUTTON_PIN = 27;    // verified original-board active-LOW test button
static constexpr uint8_t BATTERY_25_LED_PIN = 16;
static constexpr uint8_t BATTERY_50_LED_PIN = 17;
static constexpr uint8_t BATTERY_75_LED_PIN = 18;
static constexpr uint8_t BATTERY_100_LED_PIN = 19;

namespace
{
  uint32_t lastLedUpdate = 0;
  void setBatteryGauge(uint8_t percent, bool enabled)
  {
    uint8_t bars = status_indicator_logic::batteryBars(enabled, percent);
    digitalWrite(BATTERY_25_LED_PIN, bars >= 1 ? HIGH : LOW);
    digitalWrite(BATTERY_50_LED_PIN, bars >= 2 ? HIGH : LOW);
    digitalWrite(BATTERY_75_LED_PIN, bars >= 3 ? HIGH : LOW);
    digitalWrite(BATTERY_100_LED_PIN, bars >= 4 ? HIGH : LOW);
  }

  void updateIndicators()
  {
    uint32_t now = millis();
    if (now - lastLedUpdate < 25)
      return;
    lastLedUpdate = now;
    const IntrusionStatus &s = intrusion_controller::status();
    const DeviceConfig &cfg = config::get();
    const float voltage = battery_level::getVoltage();
    int percent = 0;
    if (cfg.batteryMonitoringEnabled && cfg.batteryFullVoltage > cfg.batteryEmptyVoltage)
      percent = (int)(((voltage - cfg.batteryEmptyVoltage) * 100.0f /
                       (cfg.batteryFullVoltage - cfg.batteryEmptyVoltage)) +
                      0.5f);
    percent = constrain(percent, 0, 100);
    const bool batteryCritical = cfg.batteryMonitoringEnabled && voltage <= cfg.batteryCriticalVoltage;
    const bool batteryLow = cfg.batteryMonitoringEnabled && !batteryCritical && voltage <= cfg.batteryLowVoltage;
    const bool wifiConnected = wifi_manager::wifiConnected();
    const bool internetAvailable = wifi_manager::internetAvailable();

    setBatteryGauge((uint8_t)percent, cfg.batteryMonitoringEnabled);

    const auto mode = status_indicator_logic::selectMode(
      s.alarmActive, batteryCritical, batteryLow, error_handling::hasError(),
      wifiConnected, internetAvailable, s.warmingUp, wifi_manager::provisioning());
    led_indicator::set(status_indicator_logic::isOn(mode, now));
  }
  void notification(uint32_t eventId, uint32_t cycle, time_t eventEpoch, bool) { email_manager::queueIncident(eventId, cycle, eventEpoch); }
  // Store completed incidents only; live status is supplied by the dashboard
  // API and does not need to consume persistent flash records each cycle.
  void logEvent(const IntrusionStatus &s, bool closed)
  {
    if (closed)
      event_log::record(s, true);
  }
}

void setup()
{
  Serial.begin(115200);
  led_indicator::begin(STATUS_LED_PIN, true);
  pinMode(BATTERY_25_LED_PIN, OUTPUT);
  pinMode(BATTERY_50_LED_PIN, OUTPUT);
  pinMode(BATTERY_75_LED_PIN, OUTPUT);
  pinMode(BATTERY_100_LED_PIN, OUTPUT);
  setBatteryGauge(0, false);
  // GPIO23 was driven HIGH to energize the original PCB buzzer; default it OFF before networking.
  buzzer::begin(BUZZER_PIN);
  pir_sensor::begin(PIR_PIN);
  button::begin(BUTTON_PIN);
  battery_level::begin(BATTERY_PIN, 1.61f);
  error_handling::begin();
  config::begin();
  button::setLongPressMs(config::get().buttonLongPressMs);
  battery_level::setScale(config::get().batteryDividerScale);
  event_log::begin();
  intrusion_controller::begin(notification, logEvent);
  wifi_manager::begin();
  email_manager::begin();
  web_admin::begin();
  error_handling::enableWatchdog();
  Serial.println("Smart Intrusion Detection and Remote Email Alert System ready");
}

void loop()
{
  pir_sensor::update();
  button::setLongPressMs(config::get().buttonLongPressMs);
  button::update();
  battery_level::setScale(config::get().batteryDividerScale);
  if (config::get().batteryMonitoringEnabled)
    battery_level::update();
  wifi_manager::update();
  intrusion_controller::update();
  if (button::isLongPress())
  {
    wifi_manager::openConfigurationWindow(config::get().configWindowMs);
  }
  else if (button::isTrigger())
  {
    intrusion_controller::simulateMotion();
  }
  buzzer::update();
  web_admin::update();
  updateIndicators();
  error_handling::update();
  delay(2); // yields Wi-Fi/WebServer without blocking alarm timing
}
