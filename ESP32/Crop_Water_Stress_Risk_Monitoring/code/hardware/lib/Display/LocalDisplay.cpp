#include "LocalDisplay.h"
#include "Core.h"
#include "Pins.h"
#include <Wire.h>
namespace cws {
bool LocalDisplay::nibble(uint8_t value, bool data) {
    uint8_t b = (value & 0xf0) | (backlight ? 8 : 0) | (data ? 1 : 0);
    Wire.beginTransmission(address);
    Wire.write(b);
    Wire.write(b | 4);
    Wire.write(b);
    if (Wire.endTransmission()) {
        present = false;
        return false;
    }
    delayMicroseconds(50);
    return true;
}
void LocalDisplay::send(uint8_t v, bool data) {
    if (!present)
        return;
    nibble(v, data);
    nibble(v << 4, data);
}
void LocalDisplay::line(uint8_t row, const String &t) {
    const uint8_t offsets[] = {0, 0x40, 0x14, 0x54};
    send(0x80 | offsets[row]);
    for (int i = 0; i < 20; i++)
        send(i < int(t.length()) ? (uint8_t)t[i] : ' ', true);
}
void LocalDisplay::configure(JsonVariantConst cfg) {
    present = false;
    encoder = cfg["encoder"]["enabled"];
    page = cfg["lcd"]["page"];
    address = cfg["lcd"]["address"];
    backlight = cfg["lcd"]["backlight"];
    activity = millis();
    if (encoder) {
        for (auto p : {pins::ENCODER_CLK, pins::ENCODER_DT, pins::ENCODER_SW})
            pinMode(p, INPUT_PULLUP);
        lastAB = (digitalRead(pins::ENCODER_CLK) << 1) | digitalRead(pins::ENCODER_DT);
    } else
        for (auto p : {pins::ENCODER_CLK, pins::ENCODER_DT, pins::ENCODER_SW})
            pinMode(p, INPUT);
    if (!cfg["lcd"]["enabled"].as<bool>())
        return;
    Wire.begin(pins::LCD_SDA, pins::LCD_SCL, 100000);
    Wire.setTimeOut(20);
    Wire.beginTransmission(address);
    present = Wire.endTransmission() == 0;
    if (!present)
        return;
    delay(40);
    nibble(0x30);
    delay(5);
    nibble(0x30);
    delayMicroseconds(150);
    nibble(0x30);
    nibble(0x20);
    send(0x28);
    send(0x0c);
    send(0x06);
    send(0x01);
    delay(2);
}
bool LocalDisplay::showNotice(const String &t, uint32_t duration) {
    if (!present || t.length() > 80 || duration < 1000 || duration > 60000)
        return false;
    notice = t;
    noticeAt = activity = millis();
    noticeDuration = duration;
    return true;
}
void LocalDisplay::loop(uint32_t now, JsonVariantConst cfg, JsonVariantConst r, const String &id,
                        const String &network) {
    if (encoder) {
        static const int8_t transitions[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
        uint8_t ab = (digitalRead(pins::ENCODER_CLK) << 1) | digitalRead(pins::ENCODER_DT);
        movement += transitions[(lastAB << 2) | ab];
        lastAB = ab;
        if (abs(movement) >= 4) {
            page = (page + (movement > 0 ? 1 : 2)) % 3;
            movement = 0;
            activity = now;
        }
        bool button = digitalRead(pins::ENCODER_SW);
        if (button != lastButton && elapsed(now, buttonAt, 40)) {
            buttonAt = now;
            lastButton = button;
            if (!button) {
                page = (page + 1) % 3;
                activity = now;
            }
        }
    }
    if (!present || !elapsed(now, refreshed, 1000))
        return;
    refreshed = now;
    uint32_t timeout = cfg["lcd"]["timeout_s"].as<uint32_t>() * 1000;
    backlight =
        cfg["lcd"]["backlight"].as<bool>() && (!timeout || !elapsed(now, activity, timeout));
    if (noticeDuration && !elapsed(now, noticeAt, noticeDuration)) {
        for (int i = 0; i < 4; i++)
            line(i, notice.substring(i * 20, (i + 1) * 20));
        return;
    }
    line(0, "CWS " + id.substring(id.length() > 12 ? id.length() - 12 : 0));
    if (page == 0) {
        line(1, r["environment"]["valid"]
                    ? "Temp " + String(r["environment"]["temperature_c"].as<float>(), 1) + " C"
                    : "Environment invalid");
        line(2,
             r["soil"]["valid"]
                 ? "Soil index " + String(r["soil"]["relative_moisture_pct"].as<float>(), 1) + "%"
                 : "Soil uncalibrated");
    } else if (page == 1) {
        line(1, r["battery"]["valid"]
                    ? "Battery " + String(r["battery"]["voltage_v"].as<float>(), 2) + " V"
                    : "Battery invalid");
        line(2, r["solar"]["valid"]
                    ? "Solar " + String(r["solar"]["voltage_v"].as<float>(), 2) + " V"
                    : "Solar invalid");
    } else {
        line(1, "Uptime " + String(now / 1000) + " s");
        line(2, "Config rev " + String(cfg["revision"].as<uint32_t>()));
    }
    line(3, network);
}
void LocalDisplay::status(JsonObject o) const {
    o["present"] = present;
    o["encoder_enabled"] = encoder;
    o["backlight"] = backlight;
    o["page"] = page;
}
} // namespace cws
