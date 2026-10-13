#pragma once
#include "ProjectConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <mqtt_client.h>
namespace cws {
struct Incoming {
    bool acknowledgement;
    char *payload;
};
class Network {
    struct Update {
        char *json;
    };
    struct Publish {
        char *payload;
        char suffix[32];
        bool retained;
    };
    QueueHandle_t updates = nullptr, publishes = nullptr, incoming = nullptr;
    TaskHandle_t worker = nullptr;
    esp_mqtt_client_handle_t client = nullptr;
    String deviceId, base, rxTopic, rx;
    DynamicJsonDocument mqttConfig{10000};
    uint32_t wifiAttempt = 0, wifiBackoff = 1000;
    bool suspended = false;
    uint8_t subscriptions = 0;
    int commandSubscription = -1, ackSubscription = -1;
    std::atomic<bool> connected{false}, subscribed{false}, tlsVerified{false}, publishOk{false};
    std::atomic<uint32_t> reconnects{0}, receivedDrops{0};
    std::atomic<int> lastError{0};
    std::atomic<int> lastTelemetryMid{-1};
    std::atomic<bool> telemetryPublished{false};
    std::atomic<uint32_t> requestedGeneration{0}, appliedGeneration{0};
    static void task(void *arg);
    void run();
    static void event(void *arg, esp_event_base_t base, int32_t id, void *data);
    void onEvent(esp_mqtt_event_handle_t e);

  public:
    bool begin(const String &id, JsonVariantConst cfg);
    void configureWifi(JsonVariantConst cfg);
    bool configureMqtt(JsonVariantConst cfg);
    void loop(uint32_t now, JsonVariantConst cfg);
    bool publish(const char *suffix, const String &payload, bool retained = false);
    bool receive(Incoming &message) {
        return incoming && xQueueReceive(incoming, &message, 0) == pdTRUE;
    }
    void reconnectWifi(JsonVariantConst cfg) {
        suspended = false;
        configureWifi(cfg);
    }
    void disconnectWifi() {
        suspended = true;
        WiFi.disconnect(false, false);
    }
    bool online() const {
        return connected && subscribed;
    }
    bool wifiOnline() const {
        return WiFi.status() == WL_CONNECTED;
    }
    bool tlsValid() const {
        return tlsVerified;
    }
    bool lastPublish() const {
        return telemetryPublished;
    }
    int errorCode() const {
        return lastError.load();
    }
    bool currentConfiguration() const {
        return requestedGeneration == appliedGeneration;
    }
    void status(JsonObject out) const;
    void scan();
    void scanResults(JsonObject out);
};
} // namespace cws
