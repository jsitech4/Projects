#pragma once
#include "Arduino.h"
#include <map>
class Preferences {
    std::string space;

  public:
    inline static std::map<std::string, std::vector<uint8_t>> values;
    inline static std::string failKey;
    static void reset() {
        values.clear();
        failKey.clear();
    }
    bool begin(const char *name, bool) {
        space = name;
        return true;
    }
    void end() {}
    bool isKey(const char *key) {
        return values.count(space + "/" + key) > 0;
    }
    size_t getBytesLength(const char *key) {
        auto i = values.find(space + "/" + key);
        return i == values.end() ? 0 : i->second.size();
    }
    size_t getBytes(const char *key, void *out, size_t n) {
        auto i = values.find(space + "/" + key);
        if (i == values.end() || n < i->second.size())
            return 0;
        memcpy(out, i->second.data(), i->second.size());
        return i->second.size();
    }
    size_t putBytes(const char *key, const void *data, size_t n) {
        std::string full = space + "/" + key;
        if (full == failKey) {
            failKey.clear();
            return 0;
        }
        auto p = (const uint8_t *)data;
        values[full] = std::vector<uint8_t>(p, p + n);
        return n;
    }
    uint32_t getUInt(const char *key, uint32_t fallback) {
        uint32_t v;
        return getBytes(key, &v, 4) == 4 ? v : fallback;
    }
    size_t putUInt(const char *key, uint32_t value) {
        return putBytes(key, &value, 4);
    }
};
