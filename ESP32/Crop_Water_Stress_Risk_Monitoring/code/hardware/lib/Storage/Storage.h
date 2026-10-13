#pragma once
#include "ProjectConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
namespace cws {
class Storage {
    struct Entry {
        uint32_t index = 0;
        bool ack = false;
        char id[80]{};
    } entries[MaxRecords];
    size_t count = 0;
    uint32_t next = 1, dropped = 0, faults = 0, retries = 0;
    uint32_t reservedUntil = 0;
    Preferences counters;
    bool mounted = false;
    String lastAck;
    String path(const Entry &e) const;
    bool read(const Entry &e, String &json);
    bool erase(size_t i);

  public:
    bool begin();
    bool formatCommissioning();
    bool append(const String &json, const String &id, uint16_t retain, bool dropOldest);
    bool acknowledge(const String &id);
    bool pendingAfter(uint32_t after, String &json, uint32_t &cursor);
    void history(JsonObject out, uint32_t after, uint8_t limit);
    void statistics(JsonObject out) const;
    uint32_t pending() const;
    bool healthy() const {
        return mounted && faults == 0;
    }
    bool available() const {
        return mounted;
    }
    void retry() {
        retries++;
    }
    bool purgeAcknowledged();
};
} // namespace cws
