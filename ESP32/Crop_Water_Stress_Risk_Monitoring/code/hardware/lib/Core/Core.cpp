#include "Core.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
namespace cws {
bool elapsed(uint32_t n, uint32_t t, uint32_t d) {
    return uint32_t(n - t) >= d;
}
uint32_t crc32(const void *p, size_t n) {
    uint32_t c = ~0U;
    auto b = (const uint8_t *)p;
    while (n--) {
        c ^= *b++;
        for (int i = 0; i < 8; i++)
            c = (c >> 1) ^ (0xEDB88320U & -(c & 1));
    }
    return ~c;
}
bool constantEqual(const uint8_t *a, const uint8_t *b, size_t n) {
    volatile uint8_t v = 0;
    while (n--)
        v |= *a++ ^ *b++;
    return v == 0;
}
bool identifier(const std::string &s, size_t max) {
    if (s.empty() || s.size() > max)
        return false;
    for (unsigned char c : s)
        if (!std::isalnum(c) && c != '-' && c != '_')
            return false;
    return true;
}
bool passwordStrong(const std::string &s) {
    if (s.size() < 12 || s.size() > 96)
        return false;
    bool l = false, u = false, d = false, p = false;
    for (unsigned char c : s) {
        if (c < 32 || c > 126)
            return false;
        l |= std::islower(c);
        u |= std::isupper(c);
        d |= std::isdigit(c);
        p |= std::ispunct(c);
    }
    return l && u && d && p;
}
bool decodeEnvironment(const uint8_t *d, float &t, float &h) {
    if (uint8_t(d[0] + d[1] + d[2] + d[3]) != d[4])
        return false;
    h = ((d[0] << 8) | d[1]) * 0.1f;
    t = (((d[2] & 127) << 8) | d[3]) * 0.1f;
    if (d[2] & 128)
        t = -t;
    return h >= 0 && h <= 100 && t >= -40 && t <= 80;
}
bool soilCalibration(float d, float w) {
    return std::isfinite(d) && std::isfinite(w) && d >= 0 && w >= 0 && d <= 4095 && w <= 4095 &&
           std::fabs(d - w) >= 100;
}
float moisture(float r, float d, float w) {
    return soilCalibration(d, w) ? std::max(0.f, std::min(100.f, 100 * (r - d) / (w - d))) : NAN;
}
double voltage(double mv, double top, double bottom, double gain, double offset, double lo,
               double hi) {
    if (!std::isfinite(mv) || top <= 0 || bottom <= 0 || gain <= 0 || lo < 150 || hi > 2450 ||
        hi <= lo || mv < lo || mv > hi)
        return NAN;
    double v = mv / 1000 * (top + bottom) / bottom * gain + offset;
    return std::isfinite(v) && v >= 0 && v <= 1000 ? v : NAN;
}
void defaults(JsonDocument &d) {
    d.clear();
    deserializeJson(
        d,
        R"({"revision":0,"calibration_revision":0,"name":"Crop monitor","site_id":"","plot_id":"","sample_s":60,"upload_s":300,"health_s":300,"wifi":{"ssid":"","password":"","dhcp":true,"ip":"","gateway":"","subnet":"","dns":""},"mqtt":{"host":"","port":8883,"username":"","password":"","ca":"","prefix":"cws/v1/devices","timeout_s":15},"environment":{"enabled":true,"temperature_offset":0,"humidity_offset":0},"soil":{"enabled":true,"dry":null,"wet":null},"battery":{"top_ohm":0,"bottom_ohm":0,"gain":1,"offset":0,"min_mv":150,"max_mv":2450,"low_v":0,"critical_v":0},"solar":{"top_ohm":0,"bottom_ohm":0,"gain":1,"offset":0,"min_mv":150,"max_mv":2450,"low_v":0,"critical_v":0},"lcd":{"enabled":false,"address":39,"backlight":true,"timeout_s":60,"page":0},"encoder":{"enabled":false},"led":{"enabled":true,"active_high":true,"breathing":false},"buzzer":{"enabled":false,"passive":false,"active_high":true,"alerts":true},"storage":{"retain":256,"drop_oldest":false,"durable_ack":false}})");
}
static bool ipv4(const char *s) {
    if (!s)
        return false;
    int parts = 0, value = 0, digits = 0;
    for (;; s++) {
        if (*s == '.' || !*s) {
            if (!digits || value > 255)
                return false;
            parts++;
            value = digits = 0;
            if (!*s)
                return parts == 4;
        } else if (*s >= '0' && *s <= '9') {
            value = value * 10 + *s - '0';
            if (++digits > 3)
                return false;
        } else
            return false;
    }
}
static uint32_t ipv4Value(const char *text) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value = (value << 8) | strtoul(text, nullptr, 10);
        const char *dot = strchr(text, '.');
        if (dot)
            text = dot + 1;
    }
    return value;
}
static bool range(JsonVariantConst v, double lo, double hi) {
    return v.is<double>() && std::isfinite(v.as<double>()) && v.as<double>() >= lo &&
           v.as<double>() <= hi;
}
static bool str(JsonVariantConst v, size_t max) {
    if (!v.is<const char *>())
        return false;
    auto s = v.as<const char *>();
    if (strlen(s) > max)
        return false;
    for (; *s; s++)
        if ((unsigned char)*s < 32)
            return false;
    return true;
}
bool validate(JsonVariantConst d, std::string &e) {
    auto fail = [&](const char *s) {
        e = s;
        return false;
    };
    if (!str(d["name"], 48) || !str(d["site_id"], 48) || !str(d["plot_id"], 48))
        return fail("identity_invalid");
    for (auto key : {"sample_s", "upload_s", "health_s"})
        if (!d[key].is<uint32_t>() ||
            !range(d[key], key == std::string("health_s") ? 10 : 5, 86400))
            return fail("interval_invalid");
    auto w = d["wifi"];
    if (!str(w["ssid"], 32) || !str(w["password"], 63) || !w["dhcp"].is<bool>())
        return fail("wifi_invalid");
    auto wp = w["password"].as<const char *>();
    if (strlen(wp) > 0 && strlen(wp) < 8)
        return fail("wifi_password_length");
    if (!w["dhcp"].as<bool>()) {
        for (auto k : {"ip", "gateway", "subnet", "dns"})
            if (!ipv4(w[k]))
                return fail("static_ipv4_invalid");
        uint32_t ip = ipv4Value(w["ip"]), gateway = ipv4Value(w["gateway"]),
                 mask = ipv4Value(w["subnet"]), dns = ipv4Value(w["dns"]), host = ~mask;
        if (host < 3 || mask == 0 || (host & (host + 1)) || (ip & mask) != (gateway & mask) ||
            (ip & host) == 0 || (ip & host) == host || (gateway & host) == 0 ||
            (gateway & host) == host || ip == gateway || !dns || (ip >> 24) == 127 ||
            (ip >> 24) >= 224 || (ip & 0xffffff00U) == 0xc0a80400U)
            return fail("static_network_invalid");
    }
    auto m = d["mqtt"];
    if (!str(m["host"], 128) || !str(m["username"], 64) || !str(m["password"], 128) ||
        !m["port"].is<uint16_t>() || !range(m["port"], 1, 65535) || !range(m["timeout_s"], 5, 30))
        return fail("mqtt_invalid");
    for (const char *p = m["host"]; *p; p++)
        if (!std::isalnum((unsigned char)*p) && *p != '.' && *p != '-')
            return fail("mqtt_hostname_invalid");
    if (!m["ca"].is<const char *>() || strlen(m["ca"]) > 6000)
        return fail("ca_size");
    if (strcmp(m["prefix"] | "", "cws/v1/devices"))
        return fail("topic_namespace_not_approved");
    if (!d["environment"]["enabled"].is<bool>() ||
        !range(d["environment"]["temperature_offset"], -10, 10) ||
        !range(d["environment"]["humidity_offset"], -20, 20))
        return fail("environment_invalid");
    auto s = d["soil"];
    if (!s["enabled"].is<bool>())
        return fail("soil_invalid");
    if (!s["dry"].isNull() && !range(s["dry"], 0, 4095))
        return fail("soil_dry_invalid");
    if (!s["wet"].isNull() && !range(s["wet"], 0, 4095))
        return fail("soil_wet_invalid");
    if (!s["dry"].isNull() && !s["wet"].isNull() && !soilCalibration(s["dry"], s["wet"]))
        return fail("soil_span_invalid");
    for (auto k : {"battery", "solar"}) {
        auto p = d[k];
        if (!range(p["top_ohm"], 0, 1e8) || !range(p["bottom_ohm"], 0, 1e8) ||
            !range(p["gain"], 0.1, 10) || !range(p["offset"], -10, 10) ||
            !range(p["min_mv"], 150, 2400) || !range(p["max_mv"], 200, 2450) ||
            p["max_mv"].as<double>() <= p["min_mv"].as<double>() || !range(p["low_v"], 0, 1000) ||
            !range(p["critical_v"], 0, 1000) ||
            p["critical_v"].as<double>() > p["low_v"].as<double>())
            return fail("power_calibration_invalid");
        if ((p["top_ohm"].as<double>() == 0) != (p["bottom_ohm"].as<double>() == 0))
            return fail("divider_incomplete");
    }
    for (auto k : {"lcd", "encoder", "led", "buzzer"})
        if (!d[k]["enabled"].is<bool>())
            return fail("peripheral_invalid");
    if (!range(d["lcd"]["address"], 32, 63) || !range(d["lcd"]["timeout_s"], 0, 3600) ||
        !range(d["lcd"]["page"], 0, 2) || !d["lcd"]["backlight"].is<bool>())
        return fail("lcd_invalid");
    for (auto k : {"active_high", "breathing"})
        if (!d["led"][k].is<bool>())
            return fail("led_invalid");
    for (auto k : {"active_high", "passive", "alerts"})
        if (!d["buzzer"][k].is<bool>())
            return fail("buzzer_invalid");
    if (!d["storage"]["retain"].is<uint16_t>() || !range(d["storage"]["retain"], 8, 256) ||
        !d["storage"]["drop_oldest"].is<bool>() || !d["storage"]["durable_ack"].is<bool>())
        return fail("retention_invalid");
    return true;
}
static bool merge(JsonObject dst, JsonObjectConst src, std::string &e) {
    for (JsonPairConst p : src) {
        auto k = p.key().c_str();
        if (!dst.containsKey(k) || !strcmp(k, "revision") || !strcmp(k, "calibration_revision")) {
            e = "unknown_or_readonly_field";
            return false;
        }
        if (p.value().is<JsonObjectConst>()) {
            if (!dst[k].is<JsonObject>() ||
                !merge(dst[k].as<JsonObject>(), p.value().as<JsonObjectConst>(), e))
                return false;
        } else {
            if (dst[k].is<JsonObject>()) {
                e = "object_required";
                return false;
            }
            dst[k].set(p.value());
        }
    }
    return true;
}
bool patch(JsonDocument &d, JsonObjectConst p, std::string &e) {
    DynamicJsonDocument copy(16384);
    copy.set(d);
    if (!merge(copy.as<JsonObject>(), p, e) || copy.overflowed() ||
        !validate(copy.as<JsonVariantConst>(), e))
        return false;
    d.set(copy);
    return true;
}
void sanitized(JsonVariantConst cfg, JsonDocument &out) {
    out.set(cfg);
    for (auto k : {"wifi", "mqtt"}) {
        out[k]["password_set"] = strlen(out[k]["password"] | "") > 0;
        out[k].remove("password");
    }
    out["mqtt"]["ca_configured"] = strlen(out["mqtt"]["ca"] | "") > 0;
    out["mqtt"].remove("ca");
}
bool commandEnvelope(JsonVariantConst d, const std::string &id, int64_t epoch, std::string &e) {
    if (strcmp(d["schema_version"] | "", "1.0") || id != (d["device_id"] | "")) {
        e = "wrong_version_or_device";
        return false;
    }
    if (!identifier(d["request_id"] | "", 64) || !str(d["command"], 64) ||
        !d["args"].is<JsonObjectConst>()) {
        e = "invalid_envelope";
        return false;
    }
    if (epoch < 1700000000) {
        e = "clock_unsynchronized";
        return false;
    }
    if (!d["expires_at"].is<int64_t>() || d["expires_at"].as<int64_t>() < epoch ||
        d["expires_at"].as<int64_t>() > epoch + 300) {
        e = "expired_or_future";
        return false;
    }
    return true;
}
} // namespace cws
