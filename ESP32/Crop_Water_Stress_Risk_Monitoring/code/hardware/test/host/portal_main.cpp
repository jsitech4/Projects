#include "Portal.h"
#include <cassert>
#include <iostream>
using namespace cws;
static int dispatched = 0;
static HttpResponse handler(const HttpRequest &r) {
    dispatched++;
    assert(r.method == "GET" || r.method == "POST");
    return {200, "{\"ok\":true}"};
}
static void reset() {
    host::clock = 1000;
    host::sockets.clear();
    host::pending.clear();
    host::ap = false;
    host::station = true;
    dispatched = 0;
}
static void drain(Portal &p, int turns = 100) {
    for (int i = 0; i < turns; i++)
        p.loop(host::clock++);
}
static std::string post(const std::string &headers = "", const std::string &body = "{}") {
    return "POST /api/trial HTTP/1.1\r\nHost: 192.168.4.1\r\nContent-Type: "
           "application/json\r\nOrigin: http://192.168.4.1\r\n" +
           headers + "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}
static void http_sends_bounded_response() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    int fd = host::connection("GET /api/status HTTP/1.1\r\nHost: 192.168.4.1\r\n\r\n");
    drain(p);
    assert(dispatched == 1);
    assert(host::sockets[fd].output.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(host::sockets[fd].output.find("{\"ok\":true}") != std::string::npos);
}
static void monotonic_window_not_extended() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    host::clock = 300999;
    host::connection("GET /api/status HTTP/1.1\r\nHost: 192.168.4.1\r\n\r\n");
    p.loop(host::clock);
    assert(p.remaining(host::clock) == 1);
    p.loop(301000);
    assert(!p.open());
    assert(!host::ap);
    assert(!host::server);
    assert(host::station);
}
static void backpressure_does_not_extend_window() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    int fd = host::connection("GET / HTTP/1.1\r\nHost: 192.168.4.1\r\n\r\n");
    host::sockets[fd].blocked = true;
    drain(p, 10);
    p.loop(301000);
    assert(!p.open());
    assert(!host::sockets[fd].active);
}
static void rejects_origin() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    auto req = post();
    auto pos = req.find("Origin: http://192.168.4.1");
    req.replace(pos, 24, "Origin: http://evil.test");
    int fd = host::connection(req);
    drain(p);
    assert(dispatched == 0);
    assert(host::sockets[fd].output.find("403 Forbidden") != std::string::npos);
}
static void rejects_host_rebinding() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    int fd = host::connection("GET /api/status HTTP/1.1\r\nHost: evil.test\r\n\r\n");
    drain(p);
    assert(dispatched == 0);
    assert(host::sockets[fd].output.find("403 Forbidden") != std::string::npos);
}
static void rejects_station_interface() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    int fd = host::connection("GET /api/status HTTP/1.1\r\nHost: 192.168.4.1\r\n\r\n");
    host::sockets[fd].local = {192, 168, 1, 20};
    drain(p);
    assert(dispatched == 0);
    assert(!host::sockets[fd].active);
}
static void rejects_duplicate_length() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    int fd = host::connection(post("Content-Length: 2\r\n"));
    drain(p);
    assert(dispatched == 0);
    assert(host::sockets[fd].output.find("400 Bad Request") != std::string::npos);
}
static void rejects_oversized_body() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    int fd = host::connection(post("", std::string(12289, 'x')));
    drain(p);
    assert(dispatched == 0);
    assert(host::sockets[fd].output.find("413 Payload Too Large") != std::string::npos);
}
static void dispatches_json_post() {
    reset();
    Portal p;
    assert(p.begin("fixture", "Fixture-ap-password", handler));
    int fd = host::connection(post());
    drain(p);
    assert(dispatched == 1);
    assert(host::sockets[fd].output.find("200 OK") != std::string::npos);
}
static void fails_closed_without_credential() {
    reset();
    Portal p;
    assert(!p.begin("fixture", "", handler));
    assert(!host::ap);
    assert(!p.open());
}
int main() {
    http_sends_bounded_response();
    monotonic_window_not_extended();
    backpressure_does_not_extend_window();
    rejects_origin();
    rejects_host_rebinding();
    rejects_station_interface();
    rejects_duplicate_length();
    rejects_oversized_body();
    dispatches_json_post();
    fails_closed_without_credential();
    std::cout << "10 production HTTP/lifecycle adapter tests passed\n";
}
