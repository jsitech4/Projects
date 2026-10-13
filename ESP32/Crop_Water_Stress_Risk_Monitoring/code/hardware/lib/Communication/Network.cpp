#include "Network.h"
#include "Core.h"
#include <esp_log.h>
#include <lwip/netdb.h>
#include <time.h>
namespace cws {
bool Network::begin(const String &id, JsonVariantConst cfg) {
    deviceId = id;
    base = String(cfg["mqtt"]["prefix"] | "cws/v1/devices") + "/" + id;
    updates = xQueueCreate(2, sizeof(Update));
    publishes = xQueueCreate(4, sizeof(Publish));
    incoming = xQueueCreate(4, sizeof(Incoming));
    if (!updates || !publishes || !incoming)
        return false;
    esp_log_level_set("mqtt_client", ESP_LOG_NONE);
    esp_log_level_set("esp-tls", ESP_LOG_NONE);
    if (xTaskCreate(task, "cws-network", 8192, this, 1, &worker) != pdPASS)
        return false;
    configureWifi(cfg);
    return configureMqtt(cfg);
}
void Network::configureWifi(JsonVariantConst cfg) {
    suspended = false;
    WiFi.disconnect(false, false);
    auto w = cfg["wifi"];
    IPAddress ip, gateway, subnet, dns;
    if (!w["dhcp"].as<bool>()) {
        ip.fromString(w["ip"].as<const char *>());
        gateway.fromString(w["gateway"].as<const char *>());
        subnet.fromString(w["subnet"].as<const char *>());
        dns.fromString(w["dns"].as<const char *>());
        WiFi.config(ip, gateway, subnet, dns);
    } else
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    wifiAttempt = millis();
    wifiBackoff = 1000;
    const char *ssid = w["ssid"] | "";
    if (*ssid)
        WiFi.begin(ssid, w["password"] | "");
}
bool Network::configureMqtt(JsonVariantConst cfg) {
    if (!updates)
        return false;
    String json;
    serializeJson(cfg["mqtt"], json);
    Update u{strdup(json.c_str())};
    if (!u.json)
        return false;
    requestedGeneration++;
    if (xQueueSend(updates, &u, 0) != pdTRUE) {
        requestedGeneration--;
        free(u.json);
        return false;
    }
    return true;
}
void Network::loop(uint32_t now, JsonVariantConst cfg) {
    if (wifiOnline()) {
        wifiBackoff = 1000;
        return;
    }
    if (suspended || !strlen(cfg["wifi"]["ssid"] | ""))
        return;
    if (elapsed(now, wifiAttempt, std::max(10000U, wifiBackoff))) {
        wifiAttempt = now;
        wifiBackoff = std::min(300000U, wifiBackoff * 2);
        WiFi.begin(cfg["wifi"]["ssid"].as<const char *>(), cfg["wifi"]["password"] | "");
    }
}
void Network::task(void *arg) {
    static_cast<Network *>(arg)->run();
}
void Network::run() {
    uint32_t attempted = 0, backoff = 1000;
    bool haveConfig = false;
    for (;;) {
        Update update;
        if (xQueueReceive(updates, &update, 0) == pdTRUE) {
            if (client) {
                esp_mqtt_client_stop(client);
                esp_mqtt_client_destroy(client);
                client = nullptr;
            }
            connected = subscribed = tlsVerified = false;
            haveConfig = !deserializeJson(mqttConfig, static_cast<const char *>(update.json));
            free(update.json);
            appliedGeneration++;
            attempted = 0;
            backoff = 1000;
        }
        if (haveConfig && !client && WiFi.status() == WL_CONNECTED &&
            elapsed(millis(), attempted, backoff)) {
            attempted = millis();
            const char *host = mqttConfig["host"] | "";
            const char *ca = mqttConfig["ca"] | "";
            if (!*host || !*ca)
                lastError = 1;
            else if (time(nullptr) < 1700000000)
                lastError = 2;
            else {
                addrinfo hints{}, *resolved = nullptr;
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                int dns = getaddrinfo(host, nullptr, &hints, &resolved);
                if (dns) {
                    lastError = 3;
                } else {
                    freeaddrinfo(resolved);
                    esp_mqtt_client_config_t c{};
                    c.host = host;
                    c.port = mqttConfig["port"] | 8883;
                    c.transport = MQTT_TRANSPORT_OVER_SSL;
                    c.cert_pem = ca;
                    c.client_id = deviceId.c_str();
                    c.username = mqttConfig["username"] | "";
                    c.password = mqttConfig["password"] | "";
                    c.keepalive = 30;
                    c.disable_auto_reconnect = true;
                    c.network_timeout_ms = (mqttConfig["timeout_s"] | 15) * 1000;
                    c.buffer_size = 4096;
                    c.out_buffer_size = 4096;
                    c.task_stack = 7168;
                    client = esp_mqtt_client_init(&c);
                    if (client) {
                        esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, event, this);
                        esp_mqtt_client_start(client);
                    } else
                        lastError = 8;
                }
            }
            backoff = std::min(300000U, backoff * 2);
        }
        if (client && !connected && elapsed(millis(), attempted, 45000)) {
            esp_mqtt_client_stop(client);
            esp_mqtt_client_destroy(client);
            client = nullptr;
            attempted = millis();
        }
        Publish p;
        if (xQueueReceive(publishes, &p, 0) == pdTRUE) {
            if (client && online() && esp_mqtt_client_get_outbox_size(client) < 32768) {
                String topic = base + "/" + p.suffix;
                int result = esp_mqtt_client_enqueue(client, topic.c_str(), p.payload,
                                                     strlen(p.payload), 1, p.retained, true);
                if (!strcmp(p.suffix, "telemetry")) {
                    lastTelemetryMid = result;
                    telemetryPublished = false;
                }
                if (result < 0)
                    publishOk = false;
            } else
                publishOk = false;
            free(p.payload);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
void Network::event(void *arg, esp_event_base_t, int32_t, void *data) {
    static_cast<Network *>(arg)->onEvent((esp_mqtt_event_handle_t)data);
}
void Network::onEvent(esp_mqtt_event_handle_t e) {
    switch (e->event_id) {
    case MQTT_EVENT_CONNECTED:
        connected = true;
        tlsVerified = true;
        subscribed = false;
        subscriptions = 0;
        lastError = 0;
        reconnects++;
        commandSubscription = esp_mqtt_client_subscribe(client, (base + "/commands").c_str(), 1);
        ackSubscription = esp_mqtt_client_subscribe(client, (base + "/sync_ack").c_str(), 1);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        if (e->msg_id == commandSubscription)
            subscriptions |= 1;
        if (e->msg_id == ackSubscription)
            subscriptions |= 2;
        subscribed = subscriptions == 3;
        break;
    case MQTT_EVENT_DISCONNECTED:
        connected = subscribed = tlsVerified = false;
        break;
    case MQTT_EVENT_PUBLISHED:
        publishOk = true;
        if (e->msg_id == lastTelemetryMid.load())
            telemetryPublished = true;
        break;
    case MQTT_EVENT_ERROR:
        if (e->error_handle) {
            auto er = e->error_handle;
            if (er->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
                lastError = 6;
            else if (er->esp_tls_cert_verify_flags || er->esp_tls_last_esp_err)
                lastError = 5;
            else
                lastError = 4;
        } else
            lastError = 7;
        break;
    case MQTT_EVENT_DATA: {
        if (e->total_data_len <= 0 || e->total_data_len > int(MaxMessage) || e->retain) {
            receivedDrops++;
            rx = "";
            break;
        }
        if (e->current_data_offset == 0) {
            rx = "";
            rxTopic = "";
            rxTopic.concat(e->topic, e->topic_len);
            if (rxTopic != base + "/commands" && rxTopic != base + "/sync_ack")
                break;
            rx.reserve(e->total_data_len);
        }
        if (rxTopic != base + "/commands" && rxTopic != base + "/sync_ack")
            break;
        if (int(rx.length()) != e->current_data_offset) {
            rx = "";
            receivedDrops++;
            break;
        }
        rx.concat(e->data, e->data_len);
        if (rx.length() == size_t(e->total_data_len)) {
            Incoming message{rxTopic.endsWith("/sync_ack"), strdup(rx.c_str())};
            if (!message.payload || xQueueSend(incoming, &message, 0) != pdTRUE) {
                free(message.payload);
                receivedDrops++;
            }
            rx = "";
        }
        break;
    }
    default:
        break;
    }
}
bool Network::publish(const char *suffix, const String &payload, bool retained) {
    if (!online() || payload.length() > MaxMessage || strlen(suffix) >= 32)
        return false;
    Publish p{};
    p.payload = strdup(payload.c_str());
    if (!p.payload)
        return false;
    strlcpy(p.suffix, suffix, sizeof(p.suffix));
    p.retained = retained;
    if (xQueueSend(publishes, &p, 0) != pdTRUE) {
        free(p.payload);
        return false;
    }
    return true;
}
void Network::status(JsonObject o) const {
    o["wifi_connected"] = wifiOnline();
    o["ssid"] = WiFi.SSID();
    o["ip"] = WiFi.localIP().toString();
    o["ip_assigned"] = wifiOnline() && uint32_t(WiFi.localIP()) != 0;
    o["rssi_dbm"] = wifiOnline() ? WiFi.RSSI() : 0;
    o["mqtt_connected"] = connected.load();
    o["mqtt_subscribed"] = subscribed.load();
    o["tls_verified"] = tlsVerified.load();
    o["last_connection_error"] = lastError.load();
    o["reconnection_count"] = reconnects.load();
    o["last_publish_acknowledged_by_broker"] = publishOk.load();
    o["dropped_inbound_messages"] = receivedDrops.load();
}
void Network::scan() {
    if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
        WiFi.scanDelete();
        WiFi.scanNetworks(true, false, false, 150);
    }
}
void Network::scanResults(JsonObject o) {
    int n = WiFi.scanComplete();
    o["scanning"] = n == WIFI_SCAN_RUNNING;
    auto a = o.createNestedArray("networks");
    for (int i = 0; i < std::min(n, 20); i++) {
        auto v = a.createNestedObject();
        v["ssid"] = WiFi.SSID(i);
        v["rssi_dbm"] = WiFi.RSSI(i);
        v["secured"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
}
} // namespace cws
