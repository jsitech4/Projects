#include <Arduino.h>
#include "Pins.h"
#include "led_indicator.h"
#include "device_manager/device_manager.h"

namespace led_indicator
{
  static const uint8_t ledPin = Pins::STATUS_LED;

  static const uint8_t pwmChannel = 0;
  static const uint16_t pwmFreq = 10000;
  static const uint8_t pwmResolution = 8;

  static bool state = false;
  static bool forcedMode = false;
  static bool faultWasActive = false;

  static unsigned long lastBlink = 0;
  static const uint16_t faultBlinkIntervalMs = 500;

  static void writeBrightness(uint8_t value)
  {
    state = value > 0;
    ledcWrite(pwmChannel, value);
  }

  static void blinkUpdate(uint16_t interval)
  {
    unsigned long now = millis();

    if (now - lastBlink >= interval)
    {
      lastBlink = now;
      writeBrightness(state ? 0 : 255);
    }
  }

  void begin()
  {
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    forcedMode = false;
    faultWasActive = false;
    lastBlink = millis();
    writeBrightness(255);
  }

  void update()
  {
    const bool faultActive = device_manager::hasFault();

    // A latched fault takes priority over manual indicator control.
    if (faultActive)
    {
      if (!faultWasActive)
      {
        lastBlink = millis();
        writeBrightness(255);
      }
      else
      {
        blinkUpdate(faultBlinkIntervalMs);
      }

      faultWasActive = true;
      return;
    }

    if (faultWasActive)
    {
      faultWasActive = false;
      forcedMode = false;
    }

    if (forcedMode)
      return;

    if (!state)
      writeBrightness(255);
  }

  void on()
  {
    forcedMode = true;
    writeBrightness(255);
  }

  void off()
  {
    forcedMode = true;
    writeBrightness(0);
  }

  void resume()
  {
    forcedMode = false;
  }

  bool isOn()
  {
    return state;
  }
}
