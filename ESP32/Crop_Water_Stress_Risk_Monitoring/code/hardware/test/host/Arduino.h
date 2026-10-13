#pragma once
// Native test adapter for production persistence modules; never included by firmware builds.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#define PROGMEM
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define OUTPUT_OPEN_DRAIN 3
#define LOW 0
#define HIGH 1
namespace host {
inline uint32_t clock = 1000;
inline std::array<int, 40> modes{}, levels{};
inline std::array<int, 16> duty{};
inline std::array<int, 40> attached{};
} // namespace host
inline void pinMode(int pin, int mode) {
    host::modes.at(pin) = mode;
}
inline void digitalWrite(int pin, int level) {
    host::levels.at(pin) = level;
}
inline void ledcSetup(int, int, int) {}
inline void ledcAttachPin(int pin, int channel) {
    host::attached.at(pin) = channel;
}
inline void ledcDetachPin(int pin) {
    host::attached.at(pin) = -1;
}
inline void ledcWrite(int channel, int value) {
    host::duty.at(channel) = value;
}
inline uint32_t millis() {
    return host::clock;
}
class String : public std::string {
    size_t position = 0;

  public:
    using std::string::string;
    String() = default;
    String(const std::string &s) : std::string(s) {}
    String(const char *s) : std::string(s ? s : "") {}
    String(int v) : std::string(std::to_string(v)) {}
    String(unsigned int v) : std::string(std::to_string(v)) {}
    String(unsigned long v) : std::string(std::to_string(v)) {}
    String(unsigned long long v) : std::string(std::to_string(v)) {}
    bool isEmpty() const {
        return empty();
    }
    int indexOf(char c, int start = 0) const {
        auto n = find(c, start);
        return n == npos ? -1 : int(n);
    }
    int indexOf(const char *c, int start = 0) const {
        auto n = find(c, start);
        return n == npos ? -1 : int(n);
    }
    void toLowerCase() {
        for (char &c : *this)
            c = char(tolower((unsigned char)c));
    }
    void trim() {
        auto a = find_first_not_of(" \r\n\t"), b = find_last_not_of(" \r\n\t");
        *this = a == npos ? String() : String(substr(a, b - a + 1));
    }
    long toInt() const {
        return strtol(c_str(), nullptr, 10);
    }
    int lastIndexOf(char c) const {
        auto n = rfind(c);
        return n == npos ? -1 : int(n);
    }
    String substring(size_t from, size_t to = npos) const {
        return substr(from, to == npos ? npos : to - from);
    }
    bool endsWith(const char *s) const {
        size_t n = strlen(s);
        return size() >= n && compare(size() - n, n, s) == 0;
    }
    size_t write(uint8_t c) {
        push_back(char(c));
        return 1;
    }
    size_t write(const uint8_t *p, size_t n) {
        append((const char *)p, n);
        return n;
    }
    int read() {
        return position < size() ? uint8_t(at(position++)) : -1;
    }
    size_t readBytes(char *out, size_t n) {
        n = std::min(n, size() - position);
        memcpy(out, data() + position, n);
        position += n;
        return n;
    }
};
inline String operator+(const String &s, int n) {
    return String(std::string(s) + std::to_string(n));
}
inline size_t strlcpy(char *dst, const char *src, size_t size) {
    size_t n = strlen(src);
    if (size) {
        size_t copy = std::min(n, size - 1);
        memcpy(dst, src, copy);
        dst[copy] = 0;
    }
    return n;
}
#include <ArduinoJson.h>
inline void convertFromJson(JsonVariantConst value, String &out) {
    out = String(value.as<const char *>());
}
inline void convertToJson(const String &value, JsonVariant out) {
    out.set(value.c_str());
}
