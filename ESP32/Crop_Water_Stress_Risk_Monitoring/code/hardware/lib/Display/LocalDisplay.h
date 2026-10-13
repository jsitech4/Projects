#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace cws {
class LocalDisplay {
    bool present = false, encoder = false, backlight = true;
    uint8_t address = 39, page = 0, lastAB = 0;
    int8_t movement = 0;
    uint32_t refreshed = 0, activity = 0, buttonAt = 0;
    bool lastButton = true;
    String notice;
    uint32_t noticeAt = 0, noticeDuration = 0;
    bool nibble(uint8_t value, bool data = false);
    void send(uint8_t value, bool data = false);
    void line(uint8_t row, const String &text);

  public:
    void configure(JsonVariantConst cfg);
    void loop(uint32_t now, JsonVariantConst cfg, JsonVariantConst readings, const String &id,
              const String &network);
    bool available() const {
        return present;
    }
    bool showNotice(const String &text, uint32_t duration);
    void status(JsonObject o) const;
};
} // namespace cws
