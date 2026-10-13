#include "Configuration.h"
#include <mbedtls/x509_crt.h>
#include <memory>
namespace cws {
struct CommitPointer {
    uint32_t revision, previous;
    uint8_t slot;
};
bool Configuration::loadSlot(uint8_t slot, uint32_t revision, JsonDocument &to) {
    String key = String("slot") + slot;
    size_t n = nvs.getBytesLength(key.c_str());
    if (n < 6 || n > 16000)
        return false;
    std::unique_ptr<uint8_t[]> bytes(new uint8_t[n]);
    if (nvs.getBytes(key.c_str(), bytes.get(), n) != n)
        return false;
    uint32_t crc;
    memcpy(&crc, bytes.get(), 4);
    if (crc != crc32(bytes.get() + 4, n - 4))
        return false;
    if (deserializeJson(to, static_cast<const uint8_t *>(bytes.get()) + 4, n - 4) ||
        to["revision"].as<uint32_t>() != revision)
        return false;
    std::string error;
    return validate(to.as<JsonVariantConst>(), error);
}
bool Configuration::begin() {
    defaults(data);
    if (!nvs.begin("cws-config", false))
        return false;
    if (!nvs.isKey("active")) {
        healthy = true;
        return true;
    }
    CommitPointer p{};
    if (nvs.getBytes("active", &p, sizeof(p)) != sizeof(p) || p.slot > 1)
        return false;
    active = p.slot;
    if (loadSlot(active, p.revision, data)) {
        healthy = true;
        return true;
    }
    if (p.previous && loadSlot(1 - active, p.previous, data)) {
        active = 1 - active;
        healthy = true;
        return true;
    }
    defaults(data);
    return false;
}
bool Configuration::validateCertificate(JsonVariantConst c, String &error) {
    const char *ca = c["mqtt"]["ca"] | "";
    if (!*ca)
        return true;
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int rc = mbedtls_x509_crt_parse(&cert, (const unsigned char *)ca, strlen(ca) + 1);
    bool ok = rc == 0 && cert.ca_istrue;
    mbedtls_x509_crt_free(&cert);
    if (!ok)
        error = "ca_certificate_invalid";
    return ok;
}
bool Configuration::commit(JsonDocument &candidate, bool cal) {
    std::string error;
    String certError;
    if (!validate(candidate.as<JsonVariantConst>(), error) ||
        !validateCertificate(candidate.as<JsonVariantConst>(), certError))
        return false;
    uint32_t old = revision();
    if (old == UINT32_MAX)
        return false;
    candidate["revision"] = old + 1;
    candidate["calibration_revision"] = (data["calibration_revision"] | 0U) + (cal ? 1 : 0);
    String json;
    serializeJson(candidate, json);
    size_t n = json.length() + 4;
    std::unique_ptr<uint8_t[]> bytes(new uint8_t[n]);
    uint32_t crc = crc32(json.c_str(), json.length());
    memcpy(bytes.get(), &crc, 4);
    memcpy(bytes.get() + 4, json.c_str(), json.length());
    String key = String("slot") + (1 - active);
    if (nvs.putBytes(key.c_str(), bytes.get(), n) != n)
        return false;
    DynamicJsonDocument verify(16384);
    if (!loadSlot(1 - active, old + 1, verify))
        return false;
    CommitPointer pointer{};
    pointer.revision = old + 1;
    pointer.previous = old;
    pointer.slot = 1 - active;
    if (nvs.putBytes("active", &pointer, sizeof(pointer)) != sizeof(pointer))
        return false;
    active = pointer.slot;
    data.set(candidate);
    healthy = true;
    return true;
}
} // namespace cws
