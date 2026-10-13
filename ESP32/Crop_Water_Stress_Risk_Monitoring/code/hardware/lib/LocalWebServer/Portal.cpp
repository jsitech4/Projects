#include "Portal.h"
#include "Core.h"
#include "GeneratedAssets.h"
#include "ProjectConfig.h"
#include <errno.h>
#include <lwip/sockets.h>
namespace cws {
bool Portal::begin(const String &ssid, const char *password,
                   std::function<HttpResponse(const HttpRequest &)> cb) {
    handler = cb;
    if (strlen(password) < 12)
        return false;
    IPAddress ip(192, 168, 4, 1);
    if (!WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0)) ||
        !WiFi.softAP(ssid.c_str(), password, 1, 0, 2))
        return false;
    server.begin();
    server.setNoDelay(true);
    started = millis();
    active = true;
    return true;
}
uint32_t Portal::remaining(uint32_t now) const {
    return active && !elapsed(now, started, SetupWindowMs)
               ? (SetupWindowMs - uint32_t(now - started) + 999) / 1000
               : 0;
}
void Portal::closeClient() {
    client.stop();
    request = "";
    response = "";
    sent = 0;
}
void Portal::stop() {
    closeClient();
    server.end();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    active = false;
}
void Portal::respond(const HttpResponse &r) {
    const char *reason = r.status == 200   ? "OK"
                         : r.status == 202 ? "Accepted"
                         : r.status == 400 ? "Bad Request"
                         : r.status == 401 ? "Unauthorized"
                         : r.status == 403 ? "Forbidden"
                         : r.status == 404 ? "Not Found"
                         : r.status == 409 ? "Conflict"
                         : r.status == 413 ? "Payload Too Large"
                         : r.status == 429 ? "Too Many Requests"
                                           : "Error";
    response = "HTTP/1.1 " + String(r.status) + " " + reason + "\r\nContent-Type: " + r.type +
               "\r\nContent-Length: " + String(r.body.length()) +
               "\r\nCache-Control: no-store\r\nConnection: close\r\nX-Content-Type-Options: "
               "nosniff\r\nReferrer-Policy: no-referrer\r\nContent-Security-Policy: default-src "
               "'self'; script-src 'self'; style-src 'self'; connect-src 'self'; img-src 'self'; "
               "frame-ancestors 'none'; base-uri 'none'; form-action 'self'\r\n";
    if (r.cookie.length())
        response += "Set-Cookie: " + r.cookie + "\r\n";
    response += "\r\n" + r.body;
    sent = 0;
    request = "";
}
void Portal::parse() {
    int end = request.indexOf("\r\n\r\n");
    if (end < 0) {
        if (request.length() > 2048)
            respond({413, "{\"error\":\"headers_too_large\"}"});
        return;
    }
    if (end > 2048) {
        respond({413, "{\"error\":\"headers_too_large\"}"});
        return;
    }
    int line = request.indexOf("\r\n"), first = request.indexOf(' '),
        second = request.indexOf(' ', first + 1);
    if (first < 0 || second < 0 || second > line) {
        respond({400, "{}"});
        return;
    }
    HttpRequest r;
    r.method = request.substring(0, first);
    r.path = request.substring(first + 1, second);
    size_t length = 0;
    bool foundLength = false, jsonType = false;
    String host;
    int pos = line + 2;
    while (pos < end) {
        int next = request.indexOf("\r\n", pos);
        String header = request.substring(pos, next);
        int colon = header.indexOf(':');
        if (colon < 1) {
            respond({400, "{}"});
            return;
        }
        String key = header.substring(0, colon);
        key.toLowerCase();
        String value = header.substring(colon + 1);
        value.trim();
        if (key == "content-length") {
            if (foundLength || value.isEmpty()) {
                respond({400, "{}"});
                return;
            }
            for (char c : value)
                if (c < '0' || c > '9') {
                    respond({400, "{}"});
                    return;
                }
            length = value.toInt();
            foundLength = true;
        } else if (key == "transfer-encoding") {
            respond({400, "{}"});
            return;
        } else if (key == "cookie")
            r.cookie = value;
        else if (key == "x-csrf-token")
            r.csrf = value;
        else if (key == "origin")
            r.origin = value;
        else if (key == "host")
            host = value;
        else if (key == "content-type")
            jsonType = value == "application/json";
        pos = next + 2;
    }
    if (host != "192.168.4.1" && host != "192.168.4.1:80") {
        respond({403, "{\"error\":\"invalid_host\"}"});
        return;
    }
    if (length > MaxHttpBody) {
        respond({413, "{}"});
        return;
    }
    if (request.length() < size_t(end + 4) + length)
        return;
    if (r.method != "GET" && r.method != "POST") {
        respond({400, "{}"});
        return;
    }
    if (r.method == "POST" && (!jsonType || r.origin != "http://192.168.4.1")) {
        respond({403, "{\"error\":\"invalid_origin_or_content_type\"}"});
        return;
    }
    r.body = request.substring(end + 4, end + 4 + length);
    if (r.method == "GET" && (r.path == "/" || r.path == "/styles.css" || r.path == "/app.js")) {
        HttpResponse result;
        result.type = r.path == "/"             ? "text/html; charset=utf-8"
                      : r.path.endsWith(".css") ? "text/css"
                                                : "text/javascript";
        result.body = r.path == "/" ? PortalHtml : r.path.endsWith(".css") ? PortalCss : PortalJs;
        respond(result);
        return;
    }
    respond(handler(r));
}
void Portal::loop(uint32_t now) {
    if (!active)
        return;
    if (!remaining(now)) {
        stop();
        return;
    }
    if (!client) {
        client = server.available();
        if (!client)
            return;
        IPAddress remote = client.remoteIP();
        if (remote[0] != 192 || remote[1] != 168 || remote[2] != 4 ||
            client.localIP() != WiFi.softAPIP()) {
            closeClient();
            return;
        }
        client.setTimeout(10);
        accepted = now;
        request = "";
        response = "";
    }
    if (elapsed(now, accepted, 7000)) {
        closeClient();
        return;
    }
    if (response.length()) {
        size_t n = std::min<size_t>(512, response.length() - sent);
        // WiFiClient inherits Print::availableForWrite(), which returns zero in Arduino 2.x.
        int written = ::send(client.fd(), response.c_str() + sent, n, MSG_DONTWAIT);
        if (written > 0)
            sent += size_t(written);
        else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            closeClient();
            return;
        }
        if (sent == response.length())
            closeClient();
        return;
    }
    for (int i = 0; i < 512 && client.available(); i++) {
        request += (char)client.read();
        if (request.length() > MaxHttpBody + 2052) {
            respond({413, "{}"});
            return;
        }
    }
    parse();
}
} // namespace cws
