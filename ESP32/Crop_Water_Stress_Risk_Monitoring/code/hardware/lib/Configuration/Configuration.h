#pragma once
#include "Core.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
namespace cws {
class Configuration {
    Preferences nvs;
    uint8_t active = 0;
    bool loadSlot(uint8_t slot, uint32_t revision, JsonDocument &to);

  public:
    DynamicJsonDocument data{16384};
    bool healthy = false;
    bool begin();
    bool commit(JsonDocument &candidate, bool calibration = false);
    bool validateCertificate(JsonVariantConst candidate, String &error);
    uint32_t revision() const {
        return data["revision"] | 0U;
    }
};
} // namespace cws
