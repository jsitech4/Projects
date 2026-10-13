#include "Sensors.h"
#include "Core.h"
#include "Pins.h"
#include <algorithm>
#include <driver/gpio.h>
namespace cws {
void Sensors::begin() {
    pinMode(pins::DHT_DATA, INPUT_PULLUP);
    analogReadResolution(12);
    for (auto p : {pins::SOIL_SENSOR, pins::BATTERY_SENSE, pins::SOLAR_SENSE}) {
        pinMode(p, INPUT);
        analogSetPinAttenuation(p, ADC_11db);
    }
    calibratedAdc = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100,
                                             &adc) != ESP_ADC_CAL_VAL_DEFAULT_VREF;
}
bool Sensors::request(uint32_t now) {
    if (busy || !elapsed(now, lastRead, 2200))
        return false;
    busy = true;
    phase = 0;
    started = now;
    channel = count = 0;
    return true;
}
static uint32_t pulse(int level) {
    uint32_t start = micros();
    while (gpio_get_level(GPIO_NUM_32) == level) {
        if (uint32_t(micros() - start) > 110)
            return 0;
    }
    return uint32_t(micros() - start);
}
bool Sensors::frame(float &t, float &h) {
    uint8_t bytes[5]{};
    bool ok = true;
    // Only the bounded response capture holds off interrupts; the start pulse is scheduled.
    noInterrupts();
    pinMode(pins::DHT_DATA, INPUT_PULLUP);
    delayMicroseconds(35);
    if (!pulse(LOW) || !pulse(HIGH))
        ok = false;
    for (int i = 0; i < 40 && ok; i++) {
        uint32_t low = pulse(LOW), high = pulse(HIGH);
        if (!low || !high) {
            ok = false;
            break;
        }
        bytes[i / 8] = (bytes[i / 8] << 1) | (high > low);
    }
    interrupts();
    if (!ok) {
        envFault = "environment_timeout";
        return false;
    }
    if (!decodeEnvironment(bytes, t, h)) {
        envFault = "environment_checksum_or_range";
        return false;
    }
    envFault = "none";
    return true;
}
void Sensors::loop(uint32_t now, JsonVariantConst cfg) {
    if (!busy)
        return;
    if (phase == 0) {
        if (!cfg["environment"]["enabled"].as<bool>()) {
            envValid = false;
            envFault = "disabled";
            phase = 4;
            return;
        }
        pinMode(pins::DHT_DATA, OUTPUT_OPEN_DRAIN);
        digitalWrite(pins::DHT_DATA, LOW);
        started = now;
        phase = 1;
        return;
    }
    if (phase == 1 && elapsed(now, started, 2)) {
        float t, h;
        frame(t, h);
        lastRead = now;
        started = now;
        phase = 2;
        return;
    }
    if (phase == 2 && elapsed(now, started, 2200)) {
        pinMode(pins::DHT_DATA, OUTPUT_OPEN_DRAIN);
        digitalWrite(pins::DHT_DATA, LOW);
        started = now;
        phase = 3;
        return;
    }
    if (phase == 3 && elapsed(now, started, 2)) {
        envValid = frame(rawTemp, rawHumidity);
        envAt = lastRead = now;
        if (envValid) {
            lastTemp = rawTemp;
            lastHumidity = rawHumidity;
            validAt = now;
            hasValid = true;
        } else {
            failures++;
            rawTemp = rawHumidity = NAN;
        }
        phase = 4;
        return;
    }
    if (phase != 4)
        return;
    const int pins[] = {cws::pins::SOIL_SENSOR, cws::pins::BATTERY_SENSE, cws::pins::SOLAR_SENSE};
    samples[count++] = analogRead(pins[channel]);
    if (count < 16)
        return;
    std::sort(samples, samples + 16);
    uint32_t sum = 0;
    for (int i = 2; i < 14; i++)
        sum += samples[i];
    uint16_t raw = sum / 12;
    auto &a = analog[channel];
    a.same = abs(int(a.raw) - raw) <= 1 ? std::min(65535, int(a.same) + 1) : 0;
    a.raw = raw;
    a.mv = esp_adc_cal_raw_to_voltage(raw, &adc);
    a.clipped = raw <= 4 || raw >= 4091 || a.mv < 150 || a.mv > 2450;
    a.filtered = std::isfinite(a.filtered) ? a.filtered * 0.75f + raw * 0.25f : raw;
    count = 0;
    if (++channel == 3) {
        adcAt = now;
        busy = false;
        ready = true;
    }
}
static void number(JsonObject o, const char *k, double v) {
    if (std::isfinite(v))
        o[k] = v;
    else
        o[k] = nullptr;
}
void Sensors::readings(JsonObject out, JsonVariantConst cfg, uint32_t now) const {
    auto e = out.createNestedObject("environment");
    e["enabled"] = cfg["environment"]["enabled"];
    e["valid"] = envValid;
    e["fault_code"] = envFault;
    e["failure_count"] = failures;
    e["acquired_uptime_ms"] = envAt;
    if (hasValid)
        e["age_ms"] = uint32_t(now - validAt);
    else
        e["age_ms"] = nullptr;
    number(e, "raw_temperature_c", rawTemp);
    number(e, "raw_humidity_pct", rawHumidity);
    number(e, "temperature_c",
           envValid ? rawTemp + cfg["environment"]["temperature_offset"].as<float>() : NAN);
    number(e, "humidity_pct",
           envValid ? std::max(0.f, std::min(100.f,
                                             rawHumidity +
                                                 cfg["environment"]["humidity_offset"].as<float>()))
                    : NAN);
    number(e, "last_valid_temperature_c", lastTemp);
    number(e, "last_valid_humidity_pct", lastHumidity);
    const char *keys[] = {"soil", "battery", "solar"};
    for (int i = 0; i < 3; i++) {
        auto o = out.createNestedObject(keys[i]);
        auto &a = analog[i];
        auto c = cfg[keys[i]];
        o["raw_adc"] = a.raw;
        o["adc_mv"] = a.mv;
        o["adc_factory_calibrated"] = calibratedAdc;
        o["saturated"] = a.clipped;
        o["stuck_suspected"] = a.same >= 60;
        o["acquired_uptime_ms"] = adcAt;
        o["calibration_revision"] = cfg["calibration_revision"];
        bool valid = analogValid(i);
        const char *fault = adcAt ? (a.clipped      ? "adc_out_of_range"
                                     : a.same >= 60 ? "adc_stuck_suspected"
                                                    : "none")
                                  : "not_acquired";
        if (i == 0) {
            bool calibrated =
                !c["dry"].isNull() && !c["wet"].isNull() && soilCalibration(c["dry"], c["wet"]);
            o["calibration_valid"] = calibrated;
            o["enabled"] = c["enabled"];
            valid = valid && calibrated && c["enabled"].as<bool>();
            if (!calibrated)
                fault = "calibration_missing";
            if (!c["enabled"].as<bool>())
                fault = "disabled";
            number(o, "filtered_adc", a.filtered);
            number(o, "relative_moisture_pct",
                   valid ? moisture(a.filtered, c["dry"], c["wet"]) : NAN);
            o["vwc_pct"] = nullptr;
        } else {
            bool calibrated = c["top_ohm"].as<double>() > 0 && c["bottom_ohm"].as<double>() > 0;
            double v = voltage(a.mv, c["top_ohm"], c["bottom_ohm"], c["gain"], c["offset"],
                               c["min_mv"], c["max_mv"]);
            valid = valid && calibrated && std::isfinite(v);
            o["calibration_valid"] = calibrated;
            if (!calibrated)
                fault = "calibration_missing";
            else if (!std::isfinite(v))
                fault = "outside_validated_range";
            number(o, "voltage_v", valid ? v : NAN);
            o["low_threshold_v"] = c["low_v"];
            o["critical_threshold_v"] = c["critical_v"];
            o["low"] = valid && c["low_v"].as<double>() > 0 && v < c["low_v"].as<double>();
            o["critical"] =
                valid && c["critical_v"].as<double>() > 0 && v < c["critical_v"].as<double>();
            if (i == 1)
                o["percentage"] = nullptr;
        }
        o["valid"] = valid;
        o["fault_code"] = fault;
    }
}
} // namespace cws
