#include "Security.h"
#include "Core.h"
#include "FactorySecrets.h"
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/sha256.h>
namespace cws {
namespace {
// This fixed verifier represents the immutable developer credential.  The credential itself is
// never retained as source text, returned by the portal, written to NVS, or logged.
constexpr uint8_t DeveloperSalt[16] = {0x9d, 0x3a, 0x71, 0xc4, 0xe6, 0x08, 0xf2, 0xab,
                                       0x55, 0xd1, 0xc9, 0x0e, 0xf4, 0x7b, 0x6a, 0x82};
constexpr uint8_t DeveloperVerifier[32] = {
    0xff, 0xbd, 0xf5, 0x2f, 0x7f, 0x9c, 0x7b, 0x17, 0x8e, 0xee, 0x70,
    0xa2, 0xd3, 0x91, 0x7b, 0x0b, 0x69, 0x3a, 0x20, 0xed, 0x1a, 0x75,
    0x8b, 0x04, 0xbb, 0xf3, 0x62, 0x5e, 0xfe, 0x33, 0xfe, 0x42};
} // namespace
static bool fromHex(const char *s, uint8_t *out, size_t n) {
    if (strlen(s) != n * 2)
        return false;
    for (size_t i = 0; i < n; i++) {
        char b[3] = {s[2 * i], s[2 * i + 1], 0};
        if (!isxdigit(b[0]) || !isxdigit(b[1]))
            return false;
        out[i] = strtoul(b, nullptr, 16);
    }
    return true;
}
String Security::randomHex(size_t n) {
    String out;
    out.reserve(n * 2);
    for (size_t i = 0; i < n; i++) {
        uint8_t b = esp_random();
        char s[3];
        snprintf(s, sizeof(s), "%02x", b);
        out += s;
    }
    return out;
}
static bool derive(const String &p, const uint8_t *salt, uint32_t rounds, uint8_t *out) {
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    int rc = mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    if (!rc)
        rc = mbedtls_pkcs5_pbkdf2_hmac(&ctx, (const unsigned char *)p.c_str(), p.length(), salt, 16,
                                       rounds, 32, out);
    mbedtls_md_free(&ctx);
    return rc == 0;
}
const char *Security::apPassword() const {
    return factory_ap_password;
}
bool Security::begin() {
    if (!nvs.begin("cws-auth", false))
        return false;
    if (nvs.isKey("verifier")) {
        valid = nvs.getBytes("verifier", &verifier, sizeof(verifier)) == sizeof(verifier) &&
                verifier.iterations == PasswordIterations;
    } else {
        verifier.iterations = PasswordIterations;
        valid = fromHex(factory_admin_salt, verifier.salt, 16) &&
                fromHex(factory_admin_verifier, verifier.hash, 32);
    }
    valid = valid && strlen(factory_ap_password) >= 12 && strlen(factory_ap_password) <= 63;
    return valid;
}
void Security::authTask(void *context) {
    auto *work = static_cast<AuthJob *>(context);
    uint8_t hash[32]{};
    work->accepted = derive(work->current, work->previous.salt, PasswordIterations, hash) &&
                     constantEqual(hash, work->previous.hash, 32);
    if (work->portalLogin && !work->accepted) {
        work->developerAccepted = derive(work->current, DeveloperSalt, PasswordIterations, hash) &&
                                   constantEqual(hash, DeveloperVerifier, sizeof(hash));
        work->accepted = work->developerAccepted;
    }
    if (work->accepted && work->change) {
        esp_fill_random(work->replacement.salt, 16);
        work->replacement.iterations = PasswordIterations;
        work->replacement.changed = 1;
        work->accepted =
            derive(work->next, work->replacement.salt, PasswordIterations, work->replacement.hash);
    }
    mbedtls_platform_zeroize(hash, sizeof(hash));
    work->done.store(true);
    vTaskDelete(nullptr);
}
void Security::releaseJob() {
    if (!job)
        return;
    if (job->current.length())
        mbedtls_platform_zeroize(const_cast<char *>(job->current.c_str()), job->current.length());
    if (job->next.length())
        mbedtls_platform_zeroize(const_cast<char *>(job->next.c_str()), job->next.length());
    mbedtls_platform_zeroize(&job->previous, sizeof(job->previous));
    mbedtls_platform_zeroize(&job->replacement, sizeof(job->replacement));
    delete job;
    job = nullptr;
    jobId = "";
    cancelled = false;
}
void Security::loop() {
    if (job && job->done.load() && (cancelled || elapsed(millis(), jobStarted, 30000)))
        releaseJob();
}
bool Security::startAuth(const String &current, const String &next, const String &confirm,
                         bool change, String &id, String &error) {
    uint32_t now = millis();
    if (job) {
        error = "authentication_busy";
        return false;
    }
    if (delayMs && !elapsed(now, lastAttempt, delayMs)) {
        error = "login_throttled";
        return false;
    }
    if (!valid || current.length() > 96) {
        error = "authentication_failed";
        return false;
    }
    if (change && (next != confirm || next == current || !passwordStrong(next.c_str()))) {
        error = "password_strength_or_confirmation_invalid";
        return false;
    }
    lastAttempt = now;
    job = new AuthJob;
    job->current = current;
    job->next = next;
    job->previous = verifier;
    job->change = change;
    cancelled = false;
    jobId = randomHex(32);
    jobStarted = now;
    // Idle priority allows the scheduler and watchdog idle tasks to run during password stretching.
    if (xTaskCreate(authTask, "cws-password", 4096, job, 0, nullptr) != pdPASS) {
        releaseJob();
        error = "authentication_worker_unavailable";
        return false;
    }
    id = jobId;
    return true;
}
bool Security::startPortalAuth(const String &password, String &id, String &error) {
    uint32_t now = millis();
    if (job) {
        error = "authentication_busy";
        return false;
    }
    if (delayMs && !elapsed(now, lastAttempt, delayMs)) {
        error = "login_throttled";
        return false;
    }
    if (!valid || password.length() > 96) {
        error = "authentication_failed";
        return false;
    }
    lastAttempt = now;
    job = new AuthJob;
    job->current = password;
    job->previous = verifier;
    job->portalLogin = true;
    cancelled = false;
    jobId = randomHex(32);
    jobStarted = now;
    if (xTaskCreate(authTask, "cws-login", 4096, job, 0, nullptr) != pdPASS) {
        releaseJob();
        error = "authentication_worker_unavailable";
        return false;
    }
    id = jobId;
    return true;
}
bool Security::authResult(const String &id, bool &pending, bool &changed, bool &developerLogin,
                          String &error) {
    pending = false;
    changed = false;
    developerLogin = false;
    if (!job || cancelled || id != jobId) {
        error = "authentication_job_not_found";
        return false;
    }
    if (!job->done.load()) {
        pending = true;
        return true;
    }
    changed = job->change;
    if (!job->accepted) {
        attempts = std::min(8, int(attempts) + 1);
        delayMs = std::min(60000U, 1000U << attempts);
        error = "authentication_failed";
        releaseJob();
        return false;
    }
    if (job->developerAccepted) {
        developer = true;
        developerLogin = true;
    }
    if (changed) {
        if (nvs.putBytes("verifier", &job->replacement, sizeof(Verifier)) != sizeof(Verifier)) {
            error = "credential_write_failed";
            releaseJob();
            return false;
        }
        verifier = job->replacement;
        logout();
    } else {
        session = randomHex(32);
        csrf = randomHex(32);
        issued = millis();
    }
    attempts = 0;
    delayMs = 1000;
    releaseJob();
    return true;
}
bool Security::authorize(const String &cookie, const String &token, uint32_t now,
                         bool mutate) const {
    if (session.isEmpty() || elapsed(now, issued, SetupWindowMs))
        return false;
    String expected = "cws_session=" + session;
    int start = cookie.indexOf(expected);
    if (start < 0 || (start > 0 && cookie[start - 1] != ' ' && cookie[start - 1] != ';'))
        return false;
    int end = start + expected.length();
    if (end < int(cookie.length()) && cookie[end] != ';')
        return false;
    return !mutate || (token.length() == csrf.length() &&
                       constantEqual((const uint8_t *)token.c_str(), (const uint8_t *)csrf.c_str(),
                                     csrf.length()));
}
void Security::logout() {
    session = "";
    csrf = "";
    issued = 0;
    developer = false;
}
bool Security::authorizeDeveloper(const String &cookie, const String &token, uint32_t now,
                                  bool mutate) const {
    return developer && authorize(cookie, token, now, mutate);
}
bool Security::replace(const String &p) {
    Verifier next{};
    esp_fill_random(next.salt, 16);
    next.iterations = PasswordIterations;
    next.changed = 1;
    if (!derive(p, next.salt, next.iterations, next.hash))
        return false;
    if (nvs.putBytes("verifier", &next, sizeof(next)) != sizeof(next))
        return false;
    verifier = next;
    valid = true;
    logout();
    return true;
}
bool Security::serviceAuthorized(const String &token) const {
    uint8_t expected[32], actual[32];
    if (token.length() != 64 || !fromHex(factory_service_verifier, expected, 32))
        return false;
    mbedtls_sha256_ret((const uint8_t *)token.c_str(), token.length(), actual, 0);
    bool ok = constantEqual(expected, actual, 32);
    mbedtls_platform_zeroize(actual, sizeof(actual));
    return ok;
}
bool Security::serviceRecover(const String &token, const String &next) {
    return !job && serviceAuthorized(token) && passwordStrong(next.c_str()) && replace(next);
}
} // namespace cws
