#pragma once
#include "Commands.h"
#include "Configuration.h"
#include "Diagnostics.h"
#include "Indicators.h"
#include "LocalDisplay.h"
#include "Network.h"
#include "Portal.h"
#include "Security.h"
#include "Sensors.h"
#include "Storage.h"
#include <memory>
namespace cws {
class App {
    std::unique_ptr<DynamicJsonDocument> trial;
    String trialKind, trialRequest;
    bool trialSave = false;
    uint32_t trialStarted = 0;
    uint32_t lastSample = 0, lastUpload = 0, lastSync = 0, lastFault = 0,
             lastService = 0;
    uint32_t syncCursor = 0, sequence = 0, rebootAt = 0;
    bool syncing = false, previousOnline = false, previousWifi = false, previousPortal = false;
    String serialInput;
    uint8_t serviceAttempts = 0;
    bool serviceClosed = false;
    String trialState = "idle", trialError;
    void updateFaults(uint32_t now);
    void processTrial(uint32_t now);
    void service(uint32_t now);
    void publishDocument(const char *topic, JsonDocument &d, bool retained = false);

  public:
    Configuration config;
    Security security;
    Sensors sensors;
    Storage storage;
    Network network;
    Portal portal;
    Indicators indicators;
    LocalDisplay display;
    Diagnostics diagnostics;
    Commands commands{*this};
    DynamicJsonDocument readings{6144};
    String id, bootId, lastTelemetry;
    uint32_t bootCount = 0;
    bool storageHealthy = false;
    void begin();
    void loop();
    JsonVariantConst runtimeConfig() const {
        return trial ? trial->as<JsonVariantConst>() : config.data.as<JsonVariantConst>();
    }
    bool apply(JsonDocument &candidate, bool calibration, String &error);
    bool startTrial(JsonDocument &candidate, const String &kind, bool save, const String &requestId,
                    String &error);
    bool cancelTrial(String &error);
    bool trialActive() const {
        return bool(trial);
    }
    void envelope(JsonDocument &d, const char *type);
    void status(JsonDocument &d);
    void dashboardConfiguration(JsonObject out) const;
    void health(JsonObject out);
    void capabilities(JsonDocument &d);
    void event(const char *code);
    void sampleNow();
    void publishNow() {
        syncCursor = 0;
        syncing = true;
        lastUpload = millis();
    }
    void restart() {
        rebootAt = millis() + 1500;
        event("restart_scheduled");
    }
    HttpResponse http(const HttpRequest &r);
};
} // namespace cws
