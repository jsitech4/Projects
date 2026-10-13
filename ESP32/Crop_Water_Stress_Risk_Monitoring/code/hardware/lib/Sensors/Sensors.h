#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_adc_cal.h>
namespace cws {
class Sensors {
    struct Analog {
        uint16_t raw = 0;
        uint32_t mv = 0;
        float filtered = NAN;
        uint16_t same = 0;
        bool clipped = false;
    } analog[3];
    esp_adc_cal_characteristics_t adc{};
    bool calibratedAdc = false, busy = false, ready = false;
    uint8_t phase = 0, channel = 0, count = 0;
    uint16_t samples[16]{};
    uint32_t started = 0, lastRead = 0, envAt = 0, validAt = 0, adcAt = 0;
    uint32_t failures = 0;
    float rawTemp = NAN, rawHumidity = NAN, lastTemp = NAN, lastHumidity = NAN;
    bool envValid = false, hasValid = false;
    const char *envFault = "not_acquired";
    bool frame(float &t, float &h);

  public:
    void begin();
    bool request(uint32_t now);
    void loop(uint32_t now, JsonVariantConst cfg);
    bool takeReady() {
        bool r = ready;
        ready = false;
        return r;
    }
    bool acquiring() const {
        return busy;
    }
    bool timingCritical() const {
        return busy && (phase == 1 || phase == 3);
    }
    bool freshAnalog() const {
        return adcAt && uint32_t(millis() - adcAt) <= 120000;
    }
    void readings(JsonObject out, JsonVariantConst cfg, uint32_t now) const;
    uint16_t raw(size_t c) const {
        return analog[c].raw;
    }
    uint32_t millivolts(size_t c) const {
        return analog[c].mv;
    }
    bool analogValid(size_t c) const {
        return adcAt && !analog[c].clipped && analog[c].same < 60;
    }
};
} // namespace cws
