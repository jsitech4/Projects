#pragma once
namespace cws {
namespace pins {
constexpr int DHT_DATA = 32, SOIL_SENSOR = 34, BATTERY_SENSE = 35, SOLAR_SENSE = 39;
constexpr int LCD_SDA = 21, LCD_SCL = 22, ENCODER_CLK = 25, ENCODER_DT = 26, ENCODER_SW = 27;
constexpr int STATUS_LED = 2, BUZZER = 4;
static_assert(DHT_DATA == 32 && SOIL_SENSOR == 34 && BATTERY_SENSE == 35 && SOLAR_SENSE == 39,
              "Manufactured ADC/sensor pins");
static_assert(LCD_SDA == 21 && LCD_SCL == 22 && ENCODER_CLK == 25 && ENCODER_DT == 26 &&
                  ENCODER_SW == 27,
              "Manufactured optional pins");
static_assert(STATUS_LED == 2 && BUZZER == 4, "Manufactured indicator pins");
bool mappingValid();
} // namespace pins
} // namespace cws
