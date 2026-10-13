#include "Commands.h"
#include "App.h"
#include "Core.h"
#include "Pins.h"
#include "Version.h"
#include <time.h>
namespace cws {
const char *const CommandNames[] = {"device.get_info",
                                    "device.get_status",
                                    "device.get_health",
                                    "device.get_capabilities",
                                    "device.reboot",
                                    "device.factory_reset",
                                    "device.clear_recoverable_faults",
                                    "device.get_diagnostics",
                                    "device.sync_time",
                                    "config.get",
                                    "config.validate",
                                    "config.patch",
                                    "config.restore_defaults",
                                    "config.get_revision",
                                    "config.export",
                                    "environment.read_now",
                                    "environment.get_status",
                                    "environment.get_config",
                                    "environment.set_offsets",
                                    "environment.reset_offsets",
                                    "environment.enable",
                                    "environment.disable",
                                    "soil.read_now",
                                    "soil.get_status",
                                    "soil.get_calibration",
                                    "soil.capture_dry",
                                    "soil.capture_wet",
                                    "soil.set_calibration",
                                    "soil.validate_calibration",
                                    "soil.reset_calibration",
                                    "soil.enable",
                                    "soil.disable",
                                    "battery.read_now",
                                    "battery.get_status",
                                    "battery.get_calibration",
                                    "battery.capture_reference",
                                    "battery.set_calibration",
                                    "battery.reset_calibration",
                                    "battery.set_thresholds",
                                    "solar.read_now",
                                    "solar.get_status",
                                    "solar.get_calibration",
                                    "solar.capture_reference",
                                    "solar.set_calibration",
                                    "solar.reset_calibration",
                                    "solar.set_thresholds",
                                    "lcd.get_status",
                                    "lcd.set_backlight",
                                    "lcd.set_timeout",
                                    "lcd.set_page",
                                    "lcd.set_enabled",
                                    "lcd.show_notice",
                                    "led.get_status",
                                    "led.set_enabled",
                                    "led.set_preferences",
                                    "led.test",
                                    "led.reset_preferences",
                                    "buzzer.get_status",
                                    "buzzer.set_enabled",
                                    "buzzer.set_alert_preferences",
                                    "buzzer.mute",
                                    "buzzer.unmute",
                                    "buzzer.test",
                                    "buzzer.stop",
                                    "wifi.get_status",
                                    "wifi.scan",
                                    "wifi.configure",
                                    "wifi.test_connection",
                                    "wifi.reconnect",
                                    "wifi.disconnect",
                                    "wifi.forget_network",
                                    "mqtt.get_status",
                                    "mqtt.configure",
                                    "mqtt.test_connection",
                                    "mqtt.reconnect",
                                    "mqtt.get_config",
                                    "telemetry.read_now",
                                    "telemetry.get_last",
                                    "telemetry.get_history",
                                    "telemetry.publish_now",
                                    "telemetry.set_intervals",
                                    "telemetry.get_delivery_status",
                                    "telemetry.retry_pending",
                                    "storage.get_status",
                                    "storage.get_statistics",
                                    "storage.get_pending_count",
                                    "storage.export_records",
                                    "storage.retry_sync",
                                    "storage.set_retention",
                                    "storage.purge_acknowledged",
                                    "diagnostics.get_faults",
                                    "diagnostics.get_logs",
                                    "diagnostics.self_test",
                                    "diagnostics.get_memory",
                                    "diagnostics.get_reset_reason",
                                    "firmware.get_version",
                                    "firmware.get_update_status"};
const size_t CommandCount = sizeof(CommandNames) / sizeof(CommandNames[0]);
static bool readOnly(const String &c) {
    return c.indexOf(".get_") >= 0 || c == "config.get" || c == "config.export" ||
           c == "config.validate" || c == "soil.validate_calibration" ||
           c == "storage.export_records" || c == "diagnostics.self_test";
}
bool Commands::begin() {
    persistence = nvs.begin("cws-commands", false);
    if (!persistence)
        return false;
    if (nvs.isKey("receipts")) {
        if (nvs.getBytesLength("receipts") != sizeof(receipts) ||
            nvs.getBytes("receipts", receipts, sizeof(receipts)) != sizeof(receipts)) {
            persistence = false;
            return false;
        }
    }
    return true;
}
bool Commands::save() {
    return persistence && nvs.putBytes("receipts", receipts, sizeof(receipts)) == sizeof(receipts);
}
int Commands::receipt(const String &id) const {
    for (int i = 0; i < 32; i++)
        if (id == receipts[i].id)
            return i;
    return -1;
}
bool Commands::reserve(const String &id, int64_t expiry, String &error) {
    if (!persistence) {
        error = "replay_storage_unavailable";
        return false;
    }
    int64_t now = time(nullptr);
    for (auto &r : receipts)
        if (!r.id[0] || r.expires < now) {
            Receipt old = r;
            r = {};
            strlcpy(r.id, id.c_str(), sizeof(r.id));
            r.expires = expiry;
            strlcpy(r.status, "received", sizeof(r.status));
            strlcpy(r.error, "execution_indeterminate_after_restart", sizeof(r.error));
            if (!save()) {
                r = old;
                error = "replay_journal_write_failed";
                return false;
            }
            return true;
        }
    error = "replay_window_full";
    return false;
}
void Commands::complete(const String &id, bool ok, const String &error) {
    int index = receipt(id);
    if (index >= 0) {
        strlcpy(receipts[index].status, ok ? "applied" : "failed", sizeof(receipts[index].status));
        strlcpy(receipts[index].error, error.c_str(), sizeof(receipts[index].error));
        if (!save())
            app.diagnostics.set("command_journal_failed", true, "critical");
    }
    DynamicJsonDocument d(1024);
    app.envelope(d, "command_response");
    d["request_id"] = id;
    d["status"] = ok ? "applied" : "failed";
    d["error"] = error;
    d["configuration_revision"] = app.config.revision();
    d.createNestedObject("result");
    String json;
    serializeJson(d, json);
    app.network.publish("command_responses", json);
}
void Commands::sampleCompleted(bool stored) {
    if (pendingSample.length()) {
        complete(pendingSample, stored, stored ? "" : "record_not_stored");
        pendingSample = "";
    }
}
void Commands::receive(const char *json) {
    DynamicJsonDocument request(12288), response(16384);
    app.envelope(response, "command_response");
    response["status"] = "rejected";
    response["error"] = "invalid_json";
    response.createNestedObject("result");
    if (!deserializeJson(request, json, DeserializationOption::NestingLimit(8))) {
        String id = request["request_id"] | "";
        response["request_id"] = identifier(id.c_str()) ? id : String("");
        std::string e;
        if (!commandEnvelope(request.as<JsonVariantConst>(), app.id.c_str(), time(nullptr), e))
            response["error"] = e.c_str();
        else {
            String command = request["command"] | "";
            bool known = false;
            for (size_t i = 0; i < CommandCount; i++)
                if (command == CommandNames[i])
                    known = true;
            bool read = readOnly(command);
            int prior = read ? -1 : receipt(id);
            if (!known)
                response["error"] = "unsupported_command";
            else if (prior >= 0) {
                response["status"] = receipts[prior].status;
                response["error"] = receipts[prior].error;
                response["duplicate"] = true;
            } else if (!read &&
                       (!request["expected_revision"].is<uint32_t>() ||
                        request["expected_revision"].as<uint32_t>() != app.config.revision()))
                response["error"] = "revision_conflict";
            else if (!read && lastMutation && !elapsed(millis(), lastMutation, 1000))
                response["error"] = "command_rate_limited";
            else {
                String error;
                bool reserved = read || reserve(id, request["expires_at"], error);
                if (!reserved)
                    response["error"] = error;
                else {
                    if (!read)
                        lastMutation = millis();
                    response["error"] = "";
                    response["status"] = "applied";
                    execute(command, request["args"].as<JsonObjectConst>(), response, id);
                    if (!read) {
                        int index = receipt(id);
                        strlcpy(receipts[index].status, response["status"] | "failed",
                                sizeof(receipts[index].status));
                        strlcpy(receipts[index].error, response["error"] | "",
                                sizeof(receipts[index].error));
                        if (!save())
                            app.diagnostics.set("command_journal_failed", true, "critical");
                    }
                }
            }
        }
    }
    response["configuration_revision"] = app.config.revision();
    if (!response.containsKey("request_id"))
        response["request_id"] = "";
    if (response.overflowed() || measureJson(response) > MaxMessage) {
        response["result"].clear();
        response["status"] = "failed";
        response["error"] = "response_too_large";
    }
    String output;
    serializeJson(response, output);
    app.network.publish("command_responses", output);
    if (response["status"] == "failed" || response["status"] == "rejected")
        app.event("command_rejected_or_failed");
}
void Commands::capabilities(JsonObject o) const {
    auto commands = o.createNestedArray("commands");
    for (size_t i = 0; i < CommandCount; i++) {
        String c = CommandNames[i];
        if (c.startsWith("lcd.") && !app.display.available() && c != "lcd.get_status" &&
            c != "lcd.set_enabled")
            continue;
        commands.add(c);
    }
    o["command_authorization"] = "TLS broker identity and per-device ACL";
    o["mutations_require_expected_revision"] = true;
    o["max_command_lifetime_s"] = 300;
}
void Commands::execute(const String &command, JsonObjectConst args, JsonDocument &response,
                       const String &requestId) {
    int dot = command.indexOf('.');
    String group = command.substring(0, dot), action = command.substring(dot + 1);
    auto result = response["result"].as<JsonObject>();
    auto fail = [&](const char *error) {
        response["status"] = "rejected";
        response["error"] = error;
    };
    auto argKeys = [&](std::initializer_list<const char *> keys) {
        for (auto p : args) {
            bool found = false;
            for (auto k : keys)
                if (p.key() == k)
                    found = true;
            if (!found) {
                fail("unknown_argument");
                return false;
            }
        }
        return true;
    };
    DynamicJsonDocument candidate(16384), changes(10000);
    candidate.set(app.config.data);
    bool changed = false, calibration = false;
    if (action == "read_now") {
        if (!argKeys({}))
            return;
        if (pendingSample.length() || app.sensors.acquiring()) {
            fail("acquisition_busy");
            return;
        }
        if (!app.sensors.request(millis())) {
            fail("sensor_minimum_interval");
            return;
        }
        pendingSample = requestId;
        response["status"] = "accepted";
        result["completion"] = "measurement_acquired_and_stored";
        return;
    }
    if (group == "device") {
        if (action == "reboot" || action == "factory_reset") {
            if (!argKeys({"confirm_device_id"}) || args["confirm_device_id"] != app.id) {
                fail("confirmation_required");
                return;
            }
            if (action == "reboot") {
                app.restart();
                result["restart_scheduled"] = true;
                return;
            }
            defaults(candidate);
            String error;
            if (!app.apply(candidate, true, error)) {
                fail(error.c_str());
                return;
            }
            app.restart();
            result["measurements_and_administrator_preserved"] = true;
            return;
        }
        if (!argKeys({}))
            return;
        if (action == "get_info" || action == "get_status" || action == "get_diagnostics") {
            DynamicJsonDocument d(8192);
            app.status(d);
            result.set(d.as<JsonVariant>());
        } else if (action == "get_health")
            app.health(result);
        else if (action == "get_capabilities") {
            DynamicJsonDocument d(8192);
            app.capabilities(d);
            result.set(d.as<JsonVariant>());
        } else if (action == "clear_recoverable_faults") {
            app.diagnostics.clearRecovered();
            result["cleared_recovered_counters"] = true;
        } else if (action == "sync_time") {
            configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
            result["synchronization_requested"] = true;
            result["currently_synchronized"] = time(nullptr) >= 1700000000;
        }
        return;
    }
    if (group == "config") {
        if (action == "get" || action == "export") {
            if (!argKeys({}))
                return;
            DynamicJsonDocument d(10000);
            sanitized(app.config.data.as<JsonVariantConst>(), d);
            result.set(d.as<JsonVariant>());
            return;
        }
        if (action == "get_revision") {
            if (!argKeys({}))
                return;
            result["revision"] = app.config.revision();
            return;
        }
        if (action == "restore_defaults") {
            if (!argKeys({"confirm_device_id"}) || args["confirm_device_id"] != app.id) {
                fail("confirmation_required");
                return;
            }
            defaults(candidate);
            candidate["wifi"] = app.config.data["wifi"];
            candidate["mqtt"] = app.config.data["mqtt"];
            changed = calibration = true;
        } else {
            if (!argKeys({"patch"}) || !args["patch"].is<JsonObjectConst>()) {
                fail("patch_object_required");
                return;
            }
            if (args["patch"].containsKey("wifi") || args["patch"].containsKey("mqtt")) {
                fail("use_connection_trial_commands");
                return;
            }
            std::string error;
            if (!patch(candidate, args["patch"], error)) {
                fail(error.c_str());
                return;
            }
            if (action == "validate") {
                result["valid"] = true;
                return;
            }
            changed = true;
            calibration =
                args["patch"].containsKey("environment") || args["patch"].containsKey("soil") ||
                args["patch"].containsKey("battery") || args["patch"].containsKey("solar");
        }
    } else if (group == "environment" || group == "soil" || group == "battery" ||
               group == "solar") {
        if (action == "get_status") {
            if (!argKeys({}))
                return;
            result.set(app.readings[group].as<JsonVariant>());
            return;
        }
        if (action == "get_config" || action == "get_calibration") {
            if (!argKeys({}))
                return;
            result.set(app.config.data[group].as<JsonVariant>());
            result["calibration_revision"] = app.config.data["calibration_revision"];
            return;
        }
        if (action == "enable" || action == "disable") {
            if (!argKeys({}))
                return;
            changes[group]["enabled"] = action == "enable";
        } else if (action == "reset_offsets" || action == "reset_calibration") {
            if (!argKeys({}))
                return;
            DynamicJsonDocument d(16384);
            defaults(d);
            candidate[group] = d[group];
            changed = true;
        } else if (action == "set_offsets") {
            if (!argKeys({"temperature_offset", "humidity_offset"}))
                return;
            changes[group] = args;
        } else if (action == "set_calibration" || action == "validate_calibration") {
            if (group == "soil") {
                if (!argKeys({"dry", "wet"}) || !args["dry"].is<double>() ||
                    !args["wet"].is<double>() || !soilCalibration(args["dry"], args["wet"])) {
                    fail("invalid_soil_calibration");
                    return;
                }
                if (action == "validate_calibration") {
                    result["valid"] = true;
                    return;
                }
            } else if (!argKeys({"top_ohm", "bottom_ohm", "gain", "offset", "min_mv", "max_mv"}))
                return;
            changes[group] = args;
        } else if (action == "capture_dry" || action == "capture_wet") {
            if (!argKeys({}))
                return;
            if (!app.sensors.analogValid(0) || app.sensors.acquiring() ||
                !app.sensors.freshAnalog()) {
                fail("fresh_valid_adc_required");
                return;
            }
            changes["soil"][action == "capture_dry" ? "dry" : "wet"] = app.sensors.raw(0);
            result["captured_adc"] = app.sensors.raw(0);
        } else if (action == "capture_reference") {
            if (!argKeys({"reference_v"}) || !args["reference_v"].is<double>()) {
                fail("reference_voltage_required");
                return;
            }
            int channel = group == "battery" ? 1 : 2;
            auto c = candidate[group];
            double mv = app.sensors.millivolts(channel), ref = args["reference_v"];
            if (!app.sensors.analogValid(channel) || app.sensors.acquiring() ||
                !app.sensors.freshAnalog() || ref <= 0 || ref > 1000 ||
                c["top_ohm"].as<double>() <= 0 || c["bottom_ohm"].as<double>() <= 0 ||
                mv < c["min_mv"].as<double>() || mv > c["max_mv"].as<double>()) {
                fail("reference_or_divider_invalid");
                return;
            }
            changes[group]["gain"] =
                (ref - c["offset"].as<double>()) /
                (mv / 1000 * (c["top_ohm"].as<double>() + c["bottom_ohm"].as<double>()) /
                 c["bottom_ohm"].as<double>());
            result["captured_adc_mv"] = mv;
        } else if (action == "set_thresholds") {
            if (!argKeys({"low_v", "critical_v"}))
                return;
            changes[group] = args;
        }
        calibration = true;
    } else if (group == "lcd") {
        if (action == "get_status") {
            if (!argKeys({}))
                return;
            app.display.status(result);
            return;
        }
        if (action != "set_enabled" && !app.display.available()) {
            fail("unsupported_hardware");
            return;
        }
        if (action == "show_notice") {
            if (!argKeys({"text", "duration_ms"}) || !args["text"].is<const char *>() ||
                !args["duration_ms"].is<uint32_t>() ||
                !app.display.showNotice(args["text"].as<String>(), args["duration_ms"])) {
                fail("invalid_notice");
                return;
            }
            result["shown"] = true;
            return;
        }
        const char *key = action == "set_enabled"     ? "enabled"
                          : action == "set_backlight" ? "backlight"
                          : action == "set_page"      ? "page"
                                                      : "timeout_s";
        if (!argKeys({key}) || !args.containsKey(key)) {
            fail("argument_required");
            return;
        }
        changes["lcd"][key] = args[key];
    } else if (group == "led" || group == "buzzer") {
        if (action == "get_status") {
            if (!argKeys({}))
                return;
            app.indicators.status(result);
            result["configuration"] = app.config.data[group];
            return;
        }
        if (action == "test") {
            if (!argKeys({"duration_ms"}) || !args["duration_ms"].is<uint32_t>()) {
                fail("duration_required");
                return;
            }
            bool ok = group == "led" ? app.indicators.ledTest(args["duration_ms"])
                                     : app.indicators.beep(args["duration_ms"],
                                                           app.config.data.as<JsonVariantConst>());
            if (!ok)
                fail("test_disabled_muted_or_out_of_range");
            else
                result["started"] = true;
            return;
        }
        if (action == "stop" || action == "unmute") {
            if (!argKeys({}))
                return;
            if (action == "stop")
                app.indicators.stop();
            else
                app.indicators.unmute();
            return;
        }
        if (action == "mute") {
            if (!argKeys({"duration_s"}) || !args["duration_s"].is<uint32_t>() ||
                args["duration_s"].as<uint32_t>() < 1 ||
                args["duration_s"].as<uint32_t>() > 86400) {
                fail("mute_duration_invalid");
                return;
            }
            app.indicators.mute(args["duration_s"].as<uint32_t>() * 1000);
            return;
        }
        if (action == "reset_preferences") {
            if (!argKeys({}))
                return;
            DynamicJsonDocument d(16384);
            defaults(d);
            candidate[group] = d[group];
            changed = true;
        } else {
            if (action == "set_enabled") {
                if (!argKeys({"enabled"}))
                    return;
            } else if (action == "set_preferences") {
                if (!argKeys({"enabled", "active_high", "breathing"}))
                    return;
            } else if (!argKeys({"enabled", "passive", "active_high", "alerts"}))
                return;
            changes[group] = args;
        }
    } else if (group == "wifi" || group == "mqtt") {
        if (action == "get_status") {
            if (!argKeys({}))
                return;
            app.network.status(result);
            return;
        }
        if (action == "get_config") {
            if (!argKeys({}))
                return;
            DynamicJsonDocument d(10000);
            sanitized(app.config.data.as<JsonVariantConst>(), d);
            result.set(d[group].as<JsonVariant>());
            return;
        }
        if (action == "scan") {
            if (!argKeys({}))
                return;
            app.network.scanResults(result);
            app.network.scan();
            result["new_scan_requested"] = true;
            return;
        }
        if (action == "reconnect") {
            if (!argKeys({}))
                return;
            if (app.trialActive()) {
                fail("configuration_trial_busy");
                return;
            }
            if (group == "wifi")
                app.network.reconnectWifi(app.config.data.as<JsonVariantConst>());
            else if (!app.network.configureMqtt(app.config.data.as<JsonVariantConst>())) {
                fail("network_worker_busy");
                return;
            }
            result["reconnection_requested"] = true;
            return;
        }
        if (action == "disconnect") {
            if (!argKeys({}))
                return;
            app.network.disconnectWifi();
            return;
        }
        if (action == "forget_network") {
            if (!argKeys({"confirm_device_id"}) || args["confirm_device_id"] != app.id) {
                fail("confirmation_required");
                return;
            }
            candidate["wifi"]["ssid"] = "";
            candidate["wifi"]["password"] = "";
            String error;
            if (!app.apply(candidate, false, error)) {
                fail(error.c_str());
                return;
            }
            app.network.disconnectWifi();
            return;
        }
        if (!argKeys({"settings"}) || !args["settings"].is<JsonObjectConst>()) {
            fail("settings_object_required");
            return;
        }
        changes[group] = args["settings"];
        std::string e;
        if (!patch(candidate, changes.as<JsonObjectConst>(), e)) {
            fail(e.c_str());
            return;
        }
        String error;
        if (!app.startTrial(candidate, group, action == "configure", requestId, error)) {
            fail(error.c_str());
            return;
        }
        response["status"] = "accepted";
        result["trial"] = group;
        return;
    } else if (group == "telemetry" || group == "storage") {
        if (action == "get_last") {
            if (!argKeys({}))
                return;
            DynamicJsonDocument d(12288);
            if (app.lastTelemetry.isEmpty() || deserializeJson(d, app.lastTelemetry)) {
                fail("no_measurement");
                return;
            }
            result.set(d.as<JsonVariant>());
            return;
        }
        if (action == "get_history" || action == "export_records") {
            if (!argKeys({"after", "limit"}) || !args["after"].is<uint32_t>() ||
                (args.containsKey("limit") && args["limit"] != 1)) {
                fail("history_requires_after_and_limit_one");
                return;
            }
            app.storage.history(result, args["after"], 1);
            return;
        }
        if (action == "get_delivery_status" || action == "get_status" ||
            action == "get_statistics" || action == "get_pending_count") {
            if (!argKeys({}))
                return;
            app.storage.statistics(result);
            return;
        }
        if (action == "publish_now" || action == "retry_pending" || action == "retry_sync") {
            if (!argKeys({}))
                return;
            app.publishNow();
            result["synchronization_requested"] = true;
            return;
        }
        if (action == "purge_acknowledged") {
            if (!argKeys({"confirm_device_id"}) || args["confirm_device_id"] != app.id) {
                fail("confirmation_required");
                return;
            }
            if (!app.storage.purgeAcknowledged())
                fail("storage_write_failed");
            return;
        }
        if (action == "set_intervals") {
            if (!argKeys({"sample_s", "upload_s", "health_s"}))
                return;
            changes.set(args);
        } else if (action == "set_retention") {
            if (!argKeys({"retain", "drop_oldest", "durable_ack"}))
                return;
            changes["storage"] = args;
        }
    } else if (group == "diagnostics") {
        if (!argKeys({}))
            return;
        if (action == "get_faults")
            app.diagnostics.describe(result.createNestedArray("faults"));
        else if (action == "get_logs")
            app.diagnostics.history(result.createNestedArray("logs"));
        else if (action == "self_test") {
            std::string error;
            result["pin_mapping_valid"] = pins::mappingValid();
            result["configuration_valid"] = validate(app.config.data.as<JsonVariantConst>(), error);
            result["storage_mounted"] = app.storage.available();
            result["sensor_states"] = app.readings;
            result["test_scope"] = "software_checks_and_latest_acquisition";
        } else
            app.health(result);
        return;
    } else if (group == "firmware") {
        if (!argKeys({}))
            return;
        result["version"] = CWS_VERSION;
        result["build"] = CWS_BUILD;
        result["ota_supported"] = false;
        result["update_method"] = "physical_serial_upload";
        return;
    }
    if (changes.size()) {
        std::string error;
        if (!patch(candidate, changes.as<JsonObjectConst>(), error)) {
            fail(error.c_str());
            return;
        }
        changed = true;
    }
    if (changed) {
        String error;
        if (!app.apply(candidate, calibration, error)) {
            fail(error.c_str());
            return;
        }
        result["revision"] = app.config.revision();
        result["calibration_revision"] = app.config.data["calibration_revision"];
    }
}
} // namespace cws
