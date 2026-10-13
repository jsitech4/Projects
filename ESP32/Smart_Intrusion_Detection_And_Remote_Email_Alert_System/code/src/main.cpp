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
#include "Display/led_indicator/led_indicator.h"
#include "Input/button/button.h"

// The original repository keeps each component in nested library folders rather
// than PlatformIO library roots. Compile the retained modules as one firmware
// unit while preserving that established layout.
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

namespace
{
  uint32_t lastLedUpdate = 0;
  void updateIndicators()
  {
    uint32_t now = millis();
    if (now - lastLedUpdate < 100)
      return;
    lastLedUpdate = now;
    const IntrusionStatus &s = intrusion_controller::status();
    if (s.alarmActive)
      led_indicator::set((now / 125) % 2); // rapid blink: alarm
    else if (s.warmingUp)
      led_indicator::set((now / 250) % 2); // sensor warm-up
    else if (wifi_manager::provisioning())
      led_indicator::set((now / 500) % 2); // provisioning
    else if (error_handling::hasError())
      led_indicator::set(true); // persistent fault/power on
    else
      led_indicator::set(true); // normal: system powered and armed/ready
  }
  void notification(uint32_t eventId, uint32_t cycle, bool test) { email_manager::queue(eventId, cycle, test); }
  void logEvent(const IntrusionStatus &s, bool closed) { event_log::record(s, closed); }
}

void setup()
{
  Serial.begin(115200);
  led_indicator::begin(STATUS_LED_PIN, true);
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
  Serial.println("Smart Intrusion Detection and Remote Email Alert System ready");
}

void loop()
{
  pir_sensor::update();
  button::setLongPressMs(config::get().buttonLongPressMs);
  button::update();
  battery_level::setScale(config::get().batteryDividerScale);
  if (config::get().batteryMonitoringEnabled) battery_level::update();
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
  delay(2); // yields Wi-Fi/WebServer without blocking alarm timing
}
