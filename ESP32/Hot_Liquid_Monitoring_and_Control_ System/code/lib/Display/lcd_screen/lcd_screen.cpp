#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Pins.h"
#include "lcd_screen.h"
#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "load_relay/load_relay.h"
#include "device_manager/device_manager.h"
#include "local_server/local_server.h"

namespace lcd_screen
{
  static LiquidCrystal_I2C lcd(0x27, 20, 4);

  static unsigned long lastUpdate = 0;
  static unsigned long lastScreenChange = 0;

  static uint8_t screen = 0;
  static const uint8_t screenCount = 2;

  static void printFixed(uint8_t col, uint8_t row, const String &text)
  {
    lcd.setCursor(col, row);

    String out = text;

    while (out.length() < 20 - col)
      out += ' ';

    if (out.length() > 20 - col)
      out = out.substring(0, 20 - col);

    lcd.print(out);
  }

  static void drawScreen()
  {
    device_manager::Snapshot snap = device_manager::getSnapshot();

    if (screen == 0)
    {
      printFixed(0, 0, "  LIQUID LIVE DATA  ");
      printFixed(0, 1, String("Temp: ") + (temp_sensor::isValid() ? String(temp_sensor::getTemperatureC(), 1) + " C" : "N/A"));
      printFixed(0, 2, String("Level: ") + String(snap.levelPercent, 0) + "%");
      printFixed(0, 3, String("Valve: ") + (load_relay::isOn() ? "OPEN" : "CLOSED"));
    }
    else
    {
      printFixed(0, 0, "   TANK STATUS     ");
      printFixed(0, 1, String("Temp: ") + (temp_sensor::isValid() ? String(temp_sensor::getTemperatureC(), 1) + " C" : "N/A"));
      printFixed(0, 2, String("Dist : ") + String(snap.distanceCm, 1) + " cm");
      printFixed(0, 3, String("Valve: ") + (load_relay::isOn() ? "OPEN" : "CLOSED"));
    }
  }

  void begin()
  {
    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

    lcd.init();
    lcd.backlight();
    lcd.clear();
    printFixed(0, 2, "Starting system...");
  }

  void update()
  {
    unsigned long now = millis();

    bool screenChanged = false;

    if ((now - lastScreenChange) >= 3000)
    {
      screen = (screen + 1) % screenCount;
      lastScreenChange = now;
      screenChanged = true;
    }

    if (screenChanged)
    {
      lcd.clear();
      drawScreen();
      lastUpdate = now;
      return;
    }

    if ((now - lastUpdate) >= 500)
    {
      drawScreen();
      lastUpdate = now;
    }
  }

  void setScreen(uint8_t index)
  {
    if (index >= screenCount)
      return;

    screen = index;
    lastUpdate = 0;
  }

  uint8_t getScreen()
  {
    return screen;
  }
}
