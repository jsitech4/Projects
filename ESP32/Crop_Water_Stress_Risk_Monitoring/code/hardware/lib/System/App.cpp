#include "App.h"
#include "Core.h"
#include "Version.h"
#include <esp_system.h>
#include <esp_timer.h>
#include <time.h>
namespace cws {
void App::begin() {
    Serial.begin(115200);
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(false);
    uint64_t mac = ESP.getEfuseMac();
    char identity[20];
    snprintf(identity, sizeof(identity), "CWS-%012llX", (unsigned long long)mac);
    id = identity;
    bootId = Security::randomHex(8);
    Preferences nvs;
    if (nvs.begin("cws-system", false)) {
        bootCount = nvs.getUInt("boots", 0) + 1;
        nvs.putUInt("boots", bootCount);
        nvs.end();
    }
    config.begin();
    security.begin();
    storageHealthy = storage.begin();
    sensors.begin();
    indicators.begin();
    display.configure(config.data.as<JsonVariantConst>());
    commands.begin();
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
    network.begin(id, config.data.as<JsonVariantConst>());
    if (security.provisioned())
        portal.begin("CWS-SETUP-" + id.substring(id.length() - 6), security.apPassword(),
                     [this](const HttpRequest &r) { return http(r); });
    previousPortal = portal.open();
    lastSample = millis() - config.data["sample_s"].as<uint32_t>() * 1000 + 2500;
    lastHealth = millis();
    indicators.beep(100, config.data.as<JsonVariantConst>());
    event("boot");
    event(portal.open() ? "provisioning_open" : "commissioning_required");
    Serial.println("CWS firmware ready. Credential values are never logged.");
}
void App::envelope(JsonDocument &d, const char *type) {
    d.clear();
    d["schema_version"] = CWS_SCHEMA;
    d["message_type"] = type;
    d["device_id"] = id;
    d["boot_id"] = bootId;
    d["sequence"] = ++sequence;
    d["message_id"] = id + "-" + bootId + "-" + String(sequence);
    auto t = d.createNestedObject("time");
    t["uptime_ms"] = uint64_t(esp_timer_get_time() / 1000);
    time_t now = time(nullptr);
    t["synchronized"] = now >= 1700000000;
    if (now >= 1700000000)
        t["unix_s"] = int64_t(now);
    else
        t["unix_s"] = nullptr;
}
void App::publishDocument(const char *topic, JsonDocument &d, bool retained) {
    if (d.overflowed()) {
        diagnostics.set("json_capacity_exceeded", true, "critical");
        return;
    }
    String json;
    serializeJson(d, json);
    network.publish(topic, json, retained);
}
void App::health(JsonObject o) {
    o["uptime_ms"] = uint64_t(esp_timer_get_time() / 1000);
    o["reset_reason"] = int(esp_reset_reason());
    o["boot_count"] = bootCount;
    o["free_heap"] = ESP.getFreeHeap();
    o["minimum_free_heap"] = ESP.getMinFreeHeap();
    o["flash_bytes"] = ESP.getFlashChipSize();
    o["firmware_bytes"] = ESP.getSketchSize();
    o["operating_state"] = diagnostics.critical()  ? "fault"
                           : diagnostics.warning() ? "warning"
                           : network.online()      ? "online"
                                                   : "offline_buffering";
    diagnostics.describe(o.createNestedArray("active_faults"));
}
void App::status(JsonDocument &d) {
    envelope(d, "status");
    d["firmware_version"] = CWS_VERSION;
    d["build_version"] = CWS_BUILD;
    d["name"] = config.data["name"];
    d["site_id"] = config.data["site_id"];
    d["plot_id"] = config.data["plot_id"];
    d["hardware_model"] = "ESP32-WROOM-32U";
    d["configuration_revision"] = config.revision();
    d["calibration_revision"] = config.data["calibration_revision"];
    network.status(d.createNestedObject("connectivity"));
    storage.statistics(d.createNestedObject("storage"));
    health(d.createNestedObject("health"));
    indicators.status(d.createNestedObject("indicators"));
    display.status(d.createNestedObject("local_peripherals"));
    d["setup_remaining_s"] = portal.remaining(millis());
    d["trial_state"] = trialState;
    d["trial_error"] = trialError;
    d["mqtt_host"] = config.data["mqtt"]["host"];
}
void App::capabilities(JsonDocument &d) {
    envelope(d, "capabilities");
    auto c = d.createNestedObject("capabilities");
    commands.capabilities(c);
    c["headless"] = true;
    c["lcd_present"] = display.available();
    c["encoder_enabled"] = config.data["encoder"]["enabled"];
    c["tls_required"] = true;
    c["mqtt_qos"] = 1;
    c["durable_ack_enabled"] = config.data["storage"]["durable_ack"];
    c["ota"] = false;
    c["vwc"] = false;
    c["battery_percentage"] = false;
    c["mtls"] = false;
    c["max_message_bytes"] = MaxMessage;
    c["max_history_records_per_response"] = 1;
}
void App::event(const char *code) {
    diagnostics.log(code);
    DynamicJsonDocument d(1024);
    envelope(d, "event");
    d["code"] = code;
    String json;
    serializeJson(d, json);
    storage.append(json, d["message_id"].as<String>(), config.data["storage"]["retain"],
                   config.data["storage"]["drop_oldest"]);
    network.publish("events", json);
}
void App::sampleNow() {
    sensors.request(millis());
}
bool App::apply(JsonDocument &candidate, bool cal, String &error) {
    if (trial) {
        error = "configuration_trial_busy";
        return false;
    }
    if (!config.commit(candidate, cal)) {
        error = "configuration_commit_failed";
        return false;
    }
    display.configure(config.data.as<JsonVariantConst>());
    event(cal ? "calibration_changed" : "configuration_changed");
    return true;
}
bool App::startTrial(JsonDocument &candidate, const String &kind, bool save,
                     const String &requestId, String &error) {
    if (trial) {
        error = "configuration_trial_busy";
        return false;
    }
    std::string validation;
    if (!validate(candidate.as<JsonVariantConst>(), validation)) {
        error = validation.c_str();
        return false;
    }
    if (!config.validateCertificate(candidate.as<JsonVariantConst>(), error))
        return false;
    if (kind != "wifi" && kind != "mqtt") {
        error = "trial_kind_invalid";
        return false;
    }
    if (kind == "mqtt" &&
        (!strlen(candidate["mqtt"]["host"] | "") || !strlen(candidate["mqtt"]["ca"] | ""))) {
        error = "broker_and_ca_required";
        return false;
    }
    trial.reset(new DynamicJsonDocument(16384));
    trial->set(candidate);
    trialKind = kind;
    trialSave = save;
    trialRequest = requestId;
    trialStarted = millis();
    trialState = "testing_" + kind;
    trialError = "";
    if (kind == "wifi")
        network.configureWifi(trial->as<JsonVariantConst>());
    else if (!network.configureMqtt(trial->as<JsonVariantConst>())) {
        trial.reset();
        error = "network_worker_busy";
        return false;
    }
    return true;
}
bool App::cancelTrial(String &error) {
    if (!trial) {
        error = "no_active_trial";
        return false;
    }
    String request = trialRequest;
    String kind = trialKind;
    trial.reset();
    if (kind == "wifi")
        network.configureWifi(config.data.as<JsonVariantConst>());
    else
        network.configureMqtt(config.data.as<JsonVariantConst>());
    trialState = "cancelled";
    if (request.length())
        commands.complete(request, false, "trial_cancelled");
    return true;
}
void App::processTrial(uint32_t now) {
    if (!trial)
        return;
    bool good = trialKind == "wifi" ? network.wifiOnline()
                                    : (network.currentConfiguration() && network.online());
    if (trialKind == "wifi" && good) {
        IPAddress ip = WiFi.localIP();
        if (ip[0] == 192 && ip[1] == 168 && ip[2] == 4) {
            good = false;
            trialError = "router_subnet_conflicts_with_setup";
        }
    }
    if (!elapsed(now, trialStarted, 3000))
        return;
    bool expired = elapsed(now, trialStarted, trialKind == "wifi" ? 30000 : 60000);
    if (!good && !expired)
        return;
    bool saved = false;
    if (good && trialSave) {
        saved = config.commit(*trial, false);
        if (!saved) {
            good = false;
            trialError = "configuration_commit_failed";
        }
    }
    if (!good && trialError.isEmpty())
        trialError = "connection_trial_failed";
    String kind = trialKind, request = trialRequest;
    trial.reset();
    if (!saved) {
        if (kind == "wifi")
            network.configureWifi(config.data.as<JsonVariantConst>());
        else
            network.configureMqtt(config.data.as<JsonVariantConst>());
    }
    trialState = good
                     ? (saved ? (kind == "wifi" ? "wifi_saved_backend_pending_validation" : "saved")
                              : "test_passed_restored_previous")
                     : "failed_rolled_back";
    if (request.length())
        commands.complete(request, good, trialError);
    event(good ? "configuration_trial_passed" : "configuration_trial_failed");
}
void App::updateFaults(uint32_t now) {
    if (!elapsed(now, lastFault, 1000))
        return;
    lastFault = now;
    diagnostics.set("storage_unavailable", !storage.available(), "critical");
    diagnostics.set("configuration_invalid", !config.healthy, "critical");
    diagnostics.set("commissioning_required", !security.provisioned());
    if (!readings.isNull() && readings.containsKey("environment")) {
        bool env = config.data["environment"]["enabled"].as<bool>() &&
                   !readings["environment"]["valid"].as<bool>();
        if (diagnostics.set("environment_failed", env) && env) {
            event("sensor_failure");
            if (config.data["buzzer"]["alerts"])
                indicators.beep(300, config.data.as<JsonVariantConst>());
        }
        diagnostics.set("soil_invalid", config.data["soil"]["enabled"].as<bool>() &&
                                            !readings["soil"]["valid"].as<bool>());
        diagnostics.set("battery_invalid", !readings["battery"]["valid"].as<bool>());
        diagnostics.set("solar_invalid", !readings["solar"]["valid"].as<bool>());
        bool critical = readings["battery"]["critical"] | false;
        bool low = readings["battery"]["low"] | false;
        if (diagnostics.set("battery_critical", critical, "critical") && critical) {
            event("critical_battery");
            if (config.data["buzzer"]["alerts"])
                indicators.beep(1000, config.data.as<JsonVariantConst>());
        }
        if (diagnostics.set("battery_low", low) && low) {
            event("low_battery");
            if (config.data["buzzer"]["alerts"])
                indicators.beep(200, config.data.as<JsonVariantConst>());
        }
    }
    diagnostics.set("lcd_missing",
                    config.data["lcd"]["enabled"].as<bool>() && !display.available());
    diagnostics.set("wifi_offline",
                    strlen(config.data["wifi"]["ssid"] | "") && !network.wifiOnline());
    diagnostics.set("mqtt_offline", strlen(config.data["mqtt"]["host"] | "") && !network.online());
    diagnostics.set("dns_failure", network.errorCode() == 3);
    diagnostics.set("tls_failure", network.errorCode() == 5);
    diagnostics.set("mqtt_authentication_failure", network.errorCode() == 6);
    diagnostics.set("storage_integrity_fault", storage.available() && !storage.healthy(),
                    "critical");
}
void App::service(uint32_t now) {
    if (now > 60000)
        serviceClosed = true;
    if (serviceClosed || serviceAttempts >= 3)
        return;
    for (int n = 0; n < 64 && Serial.available(); n++) {
        char c = Serial.read();
        if (c == '\n') {
            if (!elapsed(now, lastService, 3000)) {
                serialInput = "";
                return;
            }
            lastService = now;
            serviceAttempts++;
            DynamicJsonDocument d(512);
            bool valid = !deserializeJson(d, serialInput);
            serialInput = "";
            bool ok = false;
            if (valid) {
                String action = d["action"] | "";
                if (action == "recover_admin")
                    ok = security.serviceRecover(d["service_token"] | "", d["new_password"] | "");
                else if (action == "initialize_storage" &&
                         security.serviceAuthorized(d["service_token"] | ""))
                    ok = storage.formatCommissioning();
            }
            Serial.println(ok ? "service_ok" : "service_rejected");
            if (ok)
                event("physical_service_completed");
        } else if (serialInput.length() < 384)
            serialInput += c;
        else {
            serialInput = "";
            serviceAttempts++;
        }
    }
}
void App::loop() {
    uint32_t now = millis();
    if (!sensors.timingCritical())
        portal.loop(now);
    if (previousPortal && !portal.open()) {
        security.logout();
        security.cancelAuth();
        event("provisioning_closed");
    }
    previousPortal = portal.open();
    security.loop();
    network.loop(now, runtimeConfig());
    processTrial(now);
    service(now);
    if (elapsed(now, lastSample, config.data["sample_s"].as<uint32_t>() * 1000) &&
        sensors.request(now))
        lastSample = now;
    sensors.loop(now, config.data.as<JsonVariantConst>());
    if (sensors.takeReady()) {
        readings.clear();
        sensors.readings(readings.to<JsonObject>(), config.data.as<JsonVariantConst>(), now);
        DynamicJsonDocument d(12288);
        status(d);
        d["message_type"] = "telemetry";
        d["measurements"] = readings;
        lastTelemetry = "";
        serializeJson(d, lastTelemetry);
        bool stored = !d.overflowed() && storage.append(lastTelemetry, d["message_id"].as<String>(),
                                                        config.data["storage"]["retain"],
                                                        config.data["storage"]["drop_oldest"]);
        if (!stored) {
            diagnostics.set("record_not_stored", true, "critical");
        } else
            diagnostics.set("record_not_stored", false);
        commands.sampleCompleted(stored);
    }
    Incoming incoming;
    if (network.receive(incoming)) {
        if (incoming.acknowledgement) {
            DynamicJsonDocument d(512);
            if (!deserializeJson(d, incoming.payload) &&
                config.data["storage"]["durable_ack"].as<bool>() && d["schema_version"] == "1.0" &&
                d["device_id"] == id && d["durably_stored"] == true &&
                identifier(d["message_id"] | "", 79)) {
                if (storage.acknowledge(d["message_id"].as<String>()))
                    diagnostics.log("backend_acknowledged");
            }
        } else
            commands.receive(incoming.payload);
        free(incoming.payload);
    }
    if (network.wifiOnline() != previousWifi) {
        previousWifi = network.wifiOnline();
        event(previousWifi ? "wifi_connected" : "wifi_disconnected");
    }
    if (network.online() != previousOnline) {
        previousOnline = network.online();
        event(previousOnline ? "mqtt_connected" : "mqtt_disconnected");
        if (previousOnline) {
            DynamicJsonDocument c(8192);
            capabilities(c);
            publishDocument("capabilities", c, true);
            DynamicJsonDocument clean(10000);
            sanitized(config.data.as<JsonVariantConst>(), clean);
            envelope(c, "configuration");
            c["configuration"] = clean;
            publishDocument("configuration", c, true);
            publishNow();
        }
    }
    if (elapsed(now, lastUpload, config.data["upload_s"].as<uint32_t>() * 1000))
        publishNow();
    if (syncing && network.online() && elapsed(now, lastSync, 1000)) {
        lastSync = now;
        String json;
        uint32_t cursor;
        if (storage.pendingAfter(syncCursor, json, cursor)) {
            StaticJsonDocument<128> filter;
            filter["message_type"] = true;
            StaticJsonDocument<128> type;
            deserializeJson(type, json, DeserializationOption::Filter(filter));
            if (network.publish(type["message_type"] == "event" ? "events" : "telemetry", json)) {
                syncCursor = cursor;
                storage.retry();
            }
        } else
            syncing = false;
    }
    if (network.online() &&
        elapsed(now, lastHealth, config.data["health_s"].as<uint32_t>() * 1000)) {
        lastHealth = now;
        DynamicJsonDocument d(8192);
        status(d);
        publishDocument("status", d, true);
    }
    updateFaults(now);
    const char *state = diagnostics.critical()  ? "fault"
                        : diagnostics.warning() ? "warning"
                        : portal.open()         ? "provisioning"
                        : network.online()      ? "online"
                        : network.wifiOnline()  ? "connecting"
                                                : "offline_buffering";
    indicators.loop(now, config.data.as<JsonVariantConst>(), state);
    display.loop(now, config.data.as<JsonVariantConst>(), readings.as<JsonVariantConst>(), id,
                 state);
    if (rebootAt && int32_t(now - rebootAt) >= 0)
        ESP.restart();
    delay(1);
}
HttpResponse App::http(const HttpRequest &r) {
    HttpResponse response;
    DynamicJsonDocument out(12288), body(16384);
    auto error = [&](int status, const String &code) {
        response.status = status;
        out.clear();
        out["error"] = code;
    };
    bool post = r.method == "POST";
    if (post && (deserializeJson(body, r.body) || !body.is<JsonObject>())) {
        error(400, "invalid_json");
        serializeJson(out, response.body);
        return response;
    }
    if (r.path == "/api/session" && !post) {
        out["remaining_s"] = portal.remaining(millis());
        bool authenticated = security.authorize(r.cookie, "", millis(), false);
        out["authenticated"] = authenticated;
        if (authenticated) {
            out["must_change_password"] = security.mustChange();
            out["developer_mode"] = security.developerUnlocked();
            out["csrf"] = security.csrfToken();
        }
    } else if (r.path == "/api/login" && post) {
        String e, job;
        if (security.startPortalAuth(body["password"] | "", job, e)) {
            response.status = 202;
            out["job_id"] = job;
            out["pending"] = true;
        } else
            error(e == "login_throttled" ? 429 : 401, e);
    } else if (r.path == "/api/auth_result" && post) {
        bool pending = false, changed = false, developerLogin = false;
        String e;
        if (!security.authResult(body["job_id"] | "", pending, changed, developerLogin, e))
            error(401, e);
        else if (pending) {
            response.status = 202;
            out["pending"] = true;
        } else if (changed) {
            out["ok"] = true;
            response.cookie = "cws_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0";
            event("administrator_password_changed");
        } else {
            response.cookie = security.sessionCookie();
            out["must_change_password"] = security.mustChange();
            out["developer_mode"] = developerLogin;
            out["csrf"] = security.csrfToken();
            event(developerLogin ? "privileged_login" : "administrator_login");
        }
    } else if (!security.authorize(r.cookie, r.csrf, millis(), post))
        error(401, "authentication_required");
    else if (r.path == "/api/logout" && post) {
        security.logout();
        security.cancelAuth();
        response.cookie = "cws_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0";
        out["ok"] = true;
    } else if (r.path == "/api/password" && post) {
        String e, job;
        if (security.startAuth(body["current"] | "", body["next"] | "", body["confirm"] | "", true,
                               job, e)) {
            response.status = 202;
            out["job_id"] = job;
            out["pending"] = true;
        } else
            error(400, e);
    } else if (r.path == "/api/developer/calibration" &&
               security.mustChange())
        error(403, "password_replacement_required");
    else if (r.path == "/api/developer/calibration" &&
             !security.authorizeDeveloper(r.cookie, r.csrf, millis(), post))
        error(401, "developer_authentication_required");
    else if (r.path == "/api/developer/calibration" && !post) {
        out["revision"] = config.revision();
        out["calibration_revision"] = config.data["calibration_revision"];
        for (const char *group : {"environment", "soil", "battery", "solar"}) {
            out["settings"][group] = config.data[group];
            out["measurements"][group] = readings[group];
        }
    } else if (r.path == "/api/developer/calibration" && post) {
        if (!body["expected_revision"].is<uint32_t>() ||
            body["expected_revision"].as<uint32_t>() != config.revision())
            error(409, "revision_conflict");
        else if (!body["settings"].is<JsonObjectConst>())
            error(400, "developer_settings_required");
        else {
            bool permitted = true;
            for (JsonPairConst entry : body["settings"].as<JsonObjectConst>()) {
                String group = entry.key().c_str();
                if ((group != "environment" && group != "soil" && group != "battery" &&
                     group != "solar") || !entry.value().is<JsonObjectConst>()) {
                    permitted = false;
                    break;
                }
            }
            if (!permitted || !body["settings"].size())
                error(400, "developer_settings_invalid");
            else {
                DynamicJsonDocument candidate(16384);
                candidate.set(config.data);
                std::string e;
                String detail;
                if (!patch(candidate, body["settings"].as<JsonObjectConst>(), e))
                    error(400, e.c_str());
                else if (!apply(candidate, true, detail))
                    error(409, detail);
                else {
                    out["ok"] = true;
                    out["revision"] = config.revision();
                    out["calibration_revision"] = config.data["calibration_revision"];
                    event("developer_calibration_saved");
                }
            }
        }
    } else if (security.mustChange())
        error(403, "password_replacement_required");
    else if (r.path == "/api/status" && !post) {
        status(out);
        out["remaining_s"] = portal.remaining(millis());
    } else if (r.path == "/api/config" && !post) {
        sanitized(config.data.as<JsonVariantConst>(), out);
        // Calibration is deliberately available only after the separate developer unlock.
        out.remove("environment");
        out.remove("soil");
        out.remove("battery");
        out.remove("solar");
    } else if (r.path == "/api/scan") {
        if (post)
            network.scan();
        network.scanResults(out.to<JsonObject>());
    } else if (r.path == "/api/cancel" && post) {
        String e;
        if (cancelTrial(e))
            out["ok"] = true;
        else
            error(409, e);
    } else if (r.path == "/api/trial" && post) {
        String kind = body["kind"] | "";
        if (!body["expected_revision"].is<uint32_t>() ||
            body["expected_revision"].as<uint32_t>() != config.revision())
            error(409, "revision_conflict");
        else if ((kind != "wifi" && kind != "mqtt") || !body["settings"].is<JsonObject>())
            error(400, "invalid_trial");
        else {
            DynamicJsonDocument candidate(16384), changes(10000);
            candidate.set(config.data);
            changes[kind] = body["settings"];
            if (changes[kind]["password"] == "")
                changes[kind].remove("password");
            if (body["clear_password"] == true)
                changes[kind]["password"] = "";
            std::string e;
            String detail;
            if (!patch(candidate, changes.as<JsonObjectConst>(), e))
                error(400, e.c_str());
            else if (!startTrial(candidate, kind, body["save"] | false, "", detail))
                error(409, detail);
            else {
                response.status = 202;
                out["state"] = "testing";
                event("administrator_connection_trial");
            }
        }
    } else if (r.path == "/api/diagnostics" && !post) {
        network.status(out.to<JsonObject>());
        out["wifi_association"] = network.wifiOnline() ? "passed" : "not_connected";
        out["dns_tcp_tls"] = network.tlsValid() ? "passed" : "not_verified";
        out["mqtt_authentication"] = network.online() ? "passed" : "not_verified";
        out["mqtt_subscription"] = network.online() ? "passed" : "not_verified";
        out["telemetry_publication"] =
            network.lastPublish() ? "broker_acknowledged" : "not_verified";
        storage.statistics(out.createNestedObject("backend_delivery"));
    } else
        error(404, "not_found");
    if (!portal.remaining(millis())) {
        security.logout();
        error(403, "setup_window_expired");
    }
    serializeJson(out, response.body);
    return response;
}
} // namespace cws
