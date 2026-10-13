#pragma once
#include "Arduino.h"
#include <deque>
#include <map>
constexpr int WIFI_STA = 1;
struct IPAddress {
    uint8_t bytes[4]{};
    IPAddress() = default;
    IPAddress(int a, int b, int c, int d) : bytes{uint8_t(a), uint8_t(b), uint8_t(c), uint8_t(d)} {}
    uint8_t operator[](size_t i) const {
        return bytes[i];
    }
    bool operator!=(const IPAddress &other) const {
        return memcmp(bytes, other.bytes, 4) != 0;
    }
};
namespace host {
struct Socket {
    std::string input, output;
    size_t read = 0;
    bool active = true, blocked = false;
    IPAddress remote{192, 168, 4, 2}, local{192, 168, 4, 1};
};
inline std::map<int, Socket> sockets;
inline std::deque<int> pending;
inline bool ap = false, station = true, server = false;
inline int nextFd = 1;
inline int connection(const std::string &request) {
    int fd = nextFd++;
    sockets[fd].input = request;
    pending.push_back(fd);
    return fd;
}
} // namespace host
class WiFiClient {
    int descriptor = -1;

  public:
    WiFiClient() = default;
    WiFiClient(int fd) : descriptor(fd) {}
    operator bool() const {
        return descriptor >= 0 && host::sockets[descriptor].active;
    }
    void stop() {
        if (descriptor >= 0)
            host::sockets[descriptor].active = false;
        descriptor = -1;
    }
    int fd() const {
        return descriptor;
    }
    IPAddress remoteIP() const {
        return host::sockets[descriptor].remote;
    }
    IPAddress localIP() const {
        return host::sockets[descriptor].local;
    }
    void setTimeout(int) {}
    int available() const {
        auto &s = host::sockets[descriptor];
        return s.input.size() - s.read;
    }
    int read() {
        auto &s = host::sockets[descriptor];
        return s.read < s.input.size() ? s.input[s.read++] : -1;
    }
};
class WiFiServer {
  public:
    WiFiServer(int) {}
    void begin() {
        host::server = true;
    }
    void end() {
        host::server = false;
    }
    void setNoDelay(bool) {}
    WiFiClient available() {
        if (host::pending.empty())
            return {};
        int fd = host::pending.front();
        host::pending.pop_front();
        return WiFiClient(fd);
    }
};
class HostWifi {
  public:
    bool softAPConfig(IPAddress, IPAddress, IPAddress) {
        return true;
    }
    bool softAP(const char *, const char *password, int, int, int) {
        host::ap = strlen(password) >= 12;
        return host::ap;
    }
    void softAPdisconnect(bool) {
        host::ap = false;
    }
    void mode(int) {}
    IPAddress softAPIP() const {
        return {192, 168, 4, 1};
    }
};
inline HostWifi WiFi;
