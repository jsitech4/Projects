#include "Indicators.h"
#include "Core.h"
#include "Pins.h"
namespace cws {
void Indicators::begin() {
    pinMode(pins::STATUS_LED, OUTPUT);
    digitalWrite(pins::STATUS_LED, LOW);
    pinMode(pins::BUZZER, INPUT);
    ledcSetup(0, 5000, 8);
    ledcAttachPin(pins::STATUS_LED, 0);
}
bool Indicators::beep(uint32_t duration, JsonVariantConst c) {
    if (duration < 20 || duration > 3000 || !c["buzzer"]["enabled"].as<bool>() ||
        (muteDuration && !elapsed(millis(), muteStart, muteDuration)))
        return false;
    beepStart = millis();
    beepDuration = duration;
    return true;
}
bool Indicators::ledTest(uint32_t duration) {
    if (duration < 20 || duration > 3000)
        return false;
    testStart = millis();
    testDuration = duration;
    return true;
}
void Indicators::loop(uint32_t now, JsonVariantConst c, const char *op) {
    state = op;
    if (muteDuration && elapsed(now, muteStart, muteDuration))
        muteDuration = 0;
    auto b = c["buzzer"];
    bool on = b["enabled"].as<bool>() && !muteDuration && beepDuration &&
              !elapsed(now, beepStart, beepDuration);
    if (on != sounding) {
        sounding = on;
        if (b["enabled"].as<bool>()) {
            if (b["passive"].as<bool>()) {
                ledcSetup(1, 2400, 8);
                ledcAttachPin(pins::BUZZER, 1);
                ledcWrite(1, on ? 128 : (b["active_high"].as<bool>() ? 0 : 255));
            } else {
                ledcDetachPin(pins::BUZZER);
                pinMode(pins::BUZZER, OUTPUT);
                digitalWrite(pins::BUZZER, on == b["active_high"].as<bool>() ? HIGH : LOW);
            }
        } else {
            ledcDetachPin(pins::BUZZER);
            pinMode(pins::BUZZER, INPUT);
        }
    }
    uint8_t level = 0;
    uint32_t phase = now % 2000;
    if (state == "fault")
        level = now % 300 < 150 ? 255 : 0;
    else if (state == "warning")
        level = phase < 150 || (phase >= 300 && phase < 450) ? 255 : 0;
    else if (state == "provisioning")
        level = now % 1000 < 500 ? 255 : 0;
    else if (state == "connecting")
        level = now % 500 < 80 ? 255 : 0;
    else if (state == "offline_buffering")
        level = phase < 100 ? 255 : 0;
    else if (c["led"]["breathing"].as<bool>())
        level = phase < 1000 ? phase * 255 / 1000 : (2000 - phase) * 255 / 1000;
    else
        level = phase < 80 ? 255 : 0;
    if (testDuration && !elapsed(now, testStart, testDuration) && state != "fault")
        level = 255;
    if (!c["led"]["enabled"].as<bool>() && state != "fault")
        level = 0;
    ledOn = level > 0;
    ledcWrite(0, c["led"]["active_high"].as<bool>() ? level : 255 - level);
}
void Indicators::status(JsonObject o) const {
    o["led_on"] = ledOn;
    o["led_pattern"] = state;
    o["buzzer_active"] = sounding;
    o["buzzer_muted"] = muteDuration != 0;
}
} // namespace cws
