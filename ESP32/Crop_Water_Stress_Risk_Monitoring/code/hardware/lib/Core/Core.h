#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include <string>
namespace cws {
bool elapsed(uint32_t now, uint32_t then, uint32_t interval);
uint32_t crc32(const void *bytes, size_t size);
bool constantEqual(const uint8_t *a, const uint8_t *b, size_t n);
bool identifier(const std::string &s, size_t max = 64);
bool passwordStrong(const std::string &s);
bool decodeEnvironment(const uint8_t data[5], float &temp, float &humidity);
float moisture(float raw, float dry, float wet);
bool soilCalibration(float dry, float wet);
double voltage(double mv, double top, double bottom, double gain, double offset, double minMv,
               double maxMv);
void defaults(JsonDocument &d);
bool validate(JsonVariantConst d, std::string &error);
bool patch(JsonDocument &d, JsonObjectConst changes, std::string &error);
void sanitized(JsonVariantConst cfg, JsonDocument &out);
bool commandEnvelope(JsonVariantConst d, const std::string &id, int64_t epoch, std::string &error);
} // namespace cws
