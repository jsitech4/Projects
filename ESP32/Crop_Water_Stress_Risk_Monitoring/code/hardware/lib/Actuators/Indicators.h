#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace cws {
class Indicators {
    uint32_t beepStart = 0, beepDuration = 0, muteStart = 0, muteDuration = 0, testStart = 0,
             testDuration = 0;
    bool sounding = false, ledOn = false;
    bool buzzerConfigured = false, buzzerEnabled = false, buzzerPassive = false,
         buzzerActiveHigh = true;
    String state = "startup";

  public:
    void begin();
    void loop(uint32_t now, JsonVariantConst cfg, const char *operating);
    bool beep(uint32_t duration, JsonVariantConst cfg);
    void stop() {
        beepDuration = 0;
    }
    void mute(uint32_t duration) {
        muteStart = millis();
        muteDuration = duration;
        stop();
    }
    void unmute() {
        muteDuration = 0;
    }
    bool ledTest(uint32_t duration);
    void status(JsonObject out) const;
};
} // namespace cws
