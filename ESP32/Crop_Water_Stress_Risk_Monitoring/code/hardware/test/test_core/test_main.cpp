#include <Core.h>
#include <Pins.h>
#include <cmath>
#include <cstring>
#include <unity.h>
using namespace cws;
void setUp() {}
void tearDown() {}
void pins_locked() {
    TEST_ASSERT_TRUE(pins::mappingValid());
    TEST_ASSERT_EQUAL(32, pins::DHT_DATA);
    TEST_ASSERT_EQUAL(34, pins::SOIL_SENSOR);
    TEST_ASSERT_EQUAL(35, pins::BATTERY_SENSE);
    TEST_ASSERT_EQUAL(39, pins::SOLAR_SENSE);
    TEST_ASSERT_EQUAL(21, pins::LCD_SDA);
    TEST_ASSERT_EQUAL(22, pins::LCD_SCL);
    TEST_ASSERT_EQUAL(25, pins::ENCODER_CLK);
    TEST_ASSERT_EQUAL(26, pins::ENCODER_DT);
    TEST_ASSERT_EQUAL(27, pins::ENCODER_SW);
    TEST_ASSERT_EQUAL(2, pins::STATUS_LED);
    TEST_ASSERT_EQUAL(4, pins::BUZZER);
}
void timer_exact_boundary() {
    TEST_ASSERT_FALSE(elapsed(299999, 0, 300000));
    TEST_ASSERT_TRUE(elapsed(300000, 0, 300000));
    TEST_ASSERT_TRUE(elapsed(300001, 0, 300000));
}
void timer_wraparound() {
    uint32_t start = UINT32_MAX - 100;
    TEST_ASSERT_FALSE(elapsed(100, start, 300000));
    TEST_ASSERT_TRUE(elapsed(start + 300000U, start, 300000));
}
void manufacturer_frame() {
    uint8_t data[] = {2, 0x92, 1, 0x0d, 0xa2};
    float t, h;
    TEST_ASSERT_TRUE(decodeEnvironment(data, t, h));
    TEST_ASSERT_FLOAT_WITHIN(.001f, 26.9f, t);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 65.8f, h);
}
void negative_temperature() {
    uint8_t data[] = {1, 244, 128, 101, 218};
    float t, h;
    TEST_ASSERT_TRUE(decodeEnvironment(data, t, h));
    TEST_ASSERT_FLOAT_WITHIN(.001f, -10.1f, t);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 50.f, h);
}
void corrupt_checksum() {
    uint8_t data[] = {2, 0x92, 1, 0x0d, 0xa3};
    float t, h;
    TEST_ASSERT_FALSE(decodeEnvironment(data, t, h));
}
void impossible_humidity() {
    uint8_t data[] = {4, 0, 0, 0, 4};
    float t, h;
    TEST_ASSERT_FALSE(decodeEnvironment(data, t, h));
}
void moisture_both_orientations() {
    TEST_ASSERT_FLOAT_WITHIN(.01, 50, moisture(2000, 3000, 1000));
    TEST_ASSERT_FLOAT_WITHIN(.01, 50, moisture(2000, 1000, 3000));
    TEST_ASSERT_FLOAT_WITHIN(.01, 0, moisture(3500, 3000, 1000));
    TEST_ASSERT_FLOAT_WITHIN(.01, 100, moisture(500, 3000, 1000));
}
void soil_rejects_zero_span() {
    TEST_ASSERT_FALSE(soilCalibration(100, 100));
    TEST_ASSERT_FALSE(soilCalibration(100, 199));
    TEST_ASSERT_TRUE(std::isnan(moisture(200, 100, 100)));
    TEST_ASSERT_FALSE(soilCalibration(NAN, 100));
    TEST_ASSERT_FALSE(soilCalibration(-100, 100));
}
void voltage_valid_reference() {
    TEST_ASSERT_DOUBLE_WITHIN(.0001, 10, voltage(1000, 9000, 1000, 1, 0, 150, 2450));
    TEST_ASSERT_DOUBLE_WITHIN(.0001, 10.5, voltage(1000, 9000, 1000, 1.05, 0, 150, 2450));
}
void voltage_rejects_uncommissioned() {
    TEST_ASSERT_TRUE(std::isnan(voltage(1000, 0, 0, 1, 0, 150, 2450)));
    TEST_ASSERT_TRUE(std::isnan(voltage(1000, 9000, 0, 1, 0, 150, 2450)));
}
void voltage_rejects_clipping() {
    TEST_ASSERT_TRUE(std::isnan(voltage(2500, 9000, 1000, 1, 0, 150, 2450)));
    TEST_ASSERT_TRUE(std::isnan(voltage(1000, 9000, 1000, 1, 0, 150, 900)));
    TEST_ASSERT_TRUE(std::isnan(voltage(100, 9000, 1000, 1, 0, 150, 2450)));
}
void crc_known_vector() {
    const char *s = "123456789";
    TEST_ASSERT_EQUAL_HEX32(0xcbf43926, crc32(s, 9));
    TEST_ASSERT_NOT_EQUAL(crc32("record", 6), crc32("recoRd", 6));
}
void defaults_headless() {
    DynamicJsonDocument d(16384);
    defaults(d);
    std::string e;
    TEST_ASSERT_TRUE_MESSAGE(validate(d.as<JsonVariantConst>(), e), e.c_str());
    TEST_ASSERT_FALSE(d["lcd"]["enabled"]);
    TEST_ASSERT_FALSE(d["encoder"]["enabled"]);
    TEST_ASSERT_FALSE(d["buzzer"]["enabled"]);
    TEST_ASSERT_EQUAL(60, d["sample_s"].as<int>());
    TEST_ASSERT_EQUAL(300, d["upload_s"].as<int>());
    TEST_ASSERT_TRUE(d["soil"]["dry"].isNull());
}
void patch_is_transactional() {
    DynamicJsonDocument d(16384), p(512);
    defaults(d);
    deserializeJson(p, R"({"name":"changed","sample_s":0})");
    std::string e;
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
    TEST_ASSERT_EQUAL_STRING("Crop monitor", d["name"]);
    TEST_ASSERT_EQUAL(60, d["sample_s"].as<int>());
}
void rejects_unknown_fields() {
    DynamicJsonDocument d(16384), p(512);
    defaults(d);
    deserializeJson(p, R"({"pump_gpio":16})");
    std::string e;
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
}
void revisions_readonly() {
    DynamicJsonDocument d(16384), p(512);
    defaults(d);
    p["revision"] = 999;
    std::string e;
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
    p.clear();
    p["calibration_revision"] = 99;
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
}
void insecure_tls_rejected() {
    DynamicJsonDocument d(16384), p(512);
    defaults(d);
    deserializeJson(p, R"({"mqtt":{"insecure":true}})");
    std::string e;
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
}
void wildcard_namespace_rejected() {
    DynamicJsonDocument d(16384), p(512);
    defaults(d);
    deserializeJson(p, R"({"mqtt":{"prefix":"cws/#"}})");
    std::string e;
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
}
void static_ipv4_rejected() {
    DynamicJsonDocument d(16384), p(512);
    defaults(d);
    deserializeJson(
        p,
        R"({"wifi":{"dhcp":false,"ip":"999.0.0.1","gateway":"192.168.1.1","subnet":"255.255.255.0","dns":"1.1.1.1"}})");
    std::string e;
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
}
void independent_calibrations() {
    DynamicJsonDocument d(16384), p(512);
    defaults(d);
    deserializeJson(p, R"({"battery":{"top_ohm":9000,"bottom_ohm":1000}})");
    std::string e;
    TEST_ASSERT_TRUE(patch(d, p.as<JsonObjectConst>(), e));
    TEST_ASSERT_EQUAL(0, d["solar"]["top_ohm"].as<int>());
}
void sanitized_excludes_credentials() {
    DynamicJsonDocument d(16384), out(16384);
    defaults(d);
    d["wifi"]["password"] = "fixture-wifi";
    d["mqtt"]["password"] = "fixture-mqtt";
    d["mqtt"]["ca"] = "fixture-ca";
    sanitized(d.as<JsonVariantConst>(), out);
    std::string json;
    serializeJson(out, json);
    TEST_ASSERT_TRUE(json.find("fixture-") == std::string::npos);
    TEST_ASSERT_TRUE(out["wifi"]["password_set"]);
    TEST_ASSERT_TRUE(out["mqtt"]["ca_configured"]);
    TEST_ASSERT_EQUAL_STRING("fixture-wifi", d["wifi"]["password"]);
}
void password_policy() {
    TEST_ASSERT_FALSE(passwordStrong("short"));
    TEST_ASSERT_FALSE(passwordStrong("OnlyLettersWithoutDigits"));
    TEST_ASSERT_TRUE(passwordStrong("Test-only!pass123"));
    TEST_ASSERT_FALSE(passwordStrong("Test-only!\npass123"));
}
void constant_comparison() {
    uint8_t a[] = {1, 2, 3}, b[] = {1, 2, 4};
    TEST_ASSERT_TRUE(constantEqual(a, a, 3));
    TEST_ASSERT_FALSE(constantEqual(a, b, 3));
}
void command_expiration_and_target() {
    DynamicJsonDocument d(2048);
    deserializeJson(
        d,
        R"({"schema_version":"1.0","device_id":"CWS-test","request_id":"req-1","command":"device.get_info","args":{},"expires_at":1800000100})");
    std::string e;
    TEST_ASSERT_TRUE(commandEnvelope(d.as<JsonVariantConst>(), "CWS-test", 1800000000, e));
    TEST_ASSERT_FALSE(commandEnvelope(d.as<JsonVariantConst>(), "CWS-other", 1800000000, e));
    TEST_ASSERT_FALSE(commandEnvelope(d.as<JsonVariantConst>(), "CWS-test", 1800000200, e));
    TEST_ASSERT_FALSE(commandEnvelope(d.as<JsonVariantConst>(), "CWS-test", 180000000, e));
    d["expires_at"] = 1800000400;
    TEST_ASSERT_FALSE(commandEnvelope(d.as<JsonVariantConst>(), "CWS-test", 1800000000, e));
}
void path_traversal_identifier() {
    TEST_ASSERT_FALSE(identifier("../../config"));
    TEST_ASSERT_FALSE(identifier("device/other"));
    TEST_ASSERT_TRUE(identifier("CWS-AB12-boot-1"));
}
void typed_fields_required() {
    DynamicJsonDocument d(16384), p(512);
    defaults(d);
    deserializeJson(p, R"({"sample_s":"60"})");
    std::string e;
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
    deserializeJson(p, R"({"lcd":{"enabled":"true"}})");
    TEST_ASSERT_FALSE(patch(d, p.as<JsonObjectConst>(), e));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(pins_locked);
    RUN_TEST(timer_exact_boundary);
    RUN_TEST(timer_wraparound);
    RUN_TEST(manufacturer_frame);
    RUN_TEST(negative_temperature);
    RUN_TEST(corrupt_checksum);
    RUN_TEST(impossible_humidity);
    RUN_TEST(moisture_both_orientations);
    RUN_TEST(soil_rejects_zero_span);
    RUN_TEST(voltage_valid_reference);
    RUN_TEST(voltage_rejects_uncommissioned);
    RUN_TEST(voltage_rejects_clipping);
    RUN_TEST(crc_known_vector);
    RUN_TEST(defaults_headless);
    RUN_TEST(patch_is_transactional);
    RUN_TEST(rejects_unknown_fields);
    RUN_TEST(revisions_readonly);
    RUN_TEST(insecure_tls_rejected);
    RUN_TEST(wildcard_namespace_rejected);
    RUN_TEST(static_ipv4_rejected);
    RUN_TEST(independent_calibrations);
    RUN_TEST(sanitized_excludes_credentials);
    RUN_TEST(password_policy);
    RUN_TEST(constant_comparison);
    RUN_TEST(command_expiration_and_target);
    RUN_TEST(path_traversal_identifier);
    RUN_TEST(typed_fields_required);
    return UNITY_END();
}
