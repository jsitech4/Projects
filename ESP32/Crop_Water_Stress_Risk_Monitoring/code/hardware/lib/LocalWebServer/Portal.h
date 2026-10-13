#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <functional>
namespace cws {
struct HttpRequest {
    String method, path, body, cookie, csrf, origin;
};
struct HttpResponse {
    int status = 200;
    String body;
    String cookie;
    const char *type = "application/json";
    HttpResponse() = default;
    HttpResponse(int status, const String &body) : status(status), body(body) {}
};
class Portal {
    WiFiServer server{80};
    WiFiClient client;
    String request, response;
    uint32_t started = 0, accepted = 0;
    size_t sent = 0;
    bool active = false;
    std::function<HttpResponse(const HttpRequest &)> handler;
    void closeClient();
    void respond(const HttpResponse &r);
    void parse();

  public:
    bool begin(const String &ssid, const char *password,
               std::function<HttpResponse(const HttpRequest &)> callback);
    void loop(uint32_t now);
    void stop();
    bool open() const {
        return active;
    }
    uint32_t remaining(uint32_t now) const;
};
} // namespace cws
