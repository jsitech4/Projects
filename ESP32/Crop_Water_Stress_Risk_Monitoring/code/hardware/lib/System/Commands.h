#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
namespace cws {
class App;
class Commands {
    App &app;
    Preferences nvs;
    struct Receipt {
        char id[65]{};
        int64_t expires = 0;
        char status[16]{};
        char error[64]{};
    } receipts[32];
    uint32_t lastMutation = 0;
    bool persistence = false;
    String pendingSample;
    bool save();
    int receipt(const String &id) const;
    bool reserve(const String &id, int64_t expiry, String &error);
    void execute(const String &command, JsonObjectConst args, JsonDocument &response,
                 const String &requestId);

  public:
    explicit Commands(App &a) : app(a) {}
    bool begin();
    void receive(const char *json);
    void complete(const String &requestId, bool ok, const String &error = "");
    void capabilities(JsonObject out) const;
    void sampleCompleted(bool stored);
};
extern const char *const CommandNames[];
extern const size_t CommandCount;
} // namespace cws
