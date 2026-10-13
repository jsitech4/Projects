#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace cws {
class Diagnostics {
    struct Fault {
        const char *code = nullptr;
        const char *severity = nullptr;
        uint32_t since = 0, count = 0;
        bool active = false;
    } faults[24];
    struct Log {
        char code[48]{};
        uint32_t at = 0;
    } logs[32];
    uint8_t logHead = 0, logCount = 0;

  public:
    bool set(const char *code, bool active, const char *severity = "warning");
    void log(const char *code);
    void describe(JsonArray out) const;
    void history(JsonArray out) const;
    bool critical() const;
    bool warning() const;
    void clearRecovered();
};
} // namespace cws
