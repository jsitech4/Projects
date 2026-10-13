#include "Diagnostics.h"
namespace cws {
void Diagnostics::log(const char *code) {
    strlcpy(logs[logHead].code, code, sizeof(logs[logHead].code));
    logs[logHead].at = millis();
    logHead = (logHead + 1) % 32;
    logCount = std::min(32, int(logCount) + 1);
}
bool Diagnostics::set(const char *code, bool active, const char *severity) {
    for (auto &f : faults)
        if (!f.code || !strcmp(f.code, code)) {
            if (!f.code) {
                f.code = code;
                f.severity = severity;
            }
            if (f.active == active)
                return false;
            f.active = active;
            if (active) {
                f.since = millis();
                f.count++;
                log(code);
            } else
                log("fault_recovered");
            return true;
        }
    return false;
}
void Diagnostics::describe(JsonArray out) const {
    for (auto &f : faults)
        if (f.code && f.active) {
            auto o = out.createNestedObject();
            o["code"] = f.code;
            o["severity"] = f.severity;
            o["since_uptime_ms"] = f.since;
            o["occurrences_since_boot"] = f.count;
        }
}
void Diagnostics::history(JsonArray out) const {
    for (int i = 0; i < logCount; i++) {
        auto &e = logs[(logHead + 32 - logCount + i) % 32];
        auto o = out.createNestedObject();
        o["code"] = e.code;
        o["uptime_ms"] = e.at;
    }
}
bool Diagnostics::critical() const {
    for (auto &f : faults)
        if (f.active && !strcmp(f.severity, "critical"))
            return true;
    return false;
}
bool Diagnostics::warning() const {
    for (auto &f : faults)
        if (f.active)
            return true;
    return false;
}
void Diagnostics::clearRecovered() {
    for (auto &f : faults)
        if (!f.active)
            f.count = 0;
}
} // namespace cws
