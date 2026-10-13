#pragma once
#include "ProjectConfig.h"
#include <Arduino.h>
#include <Preferences.h>
#include <atomic>
namespace cws {
class Security {
    struct Verifier {
        uint8_t salt[16];
        uint8_t hash[32];
        uint32_t iterations;
        uint32_t changed;
    } verifier{};
    Preferences nvs;
    bool valid = false;
    String session, csrf;
    bool developer = false;
    uint32_t issued = 0, lastAttempt = 0, delayMs = 0;
    uint8_t attempts = 0;
    bool replace(const String &password);
    struct AuthJob {
        String current, next;
        Verifier previous{}, replacement{};
        bool change = false, portalLogin = false, developerAccepted = false, accepted = false;
        std::atomic<bool> done{false};
    };
    AuthJob *job = nullptr;
    String jobId;
    bool cancelled = false;
    uint32_t jobStarted = 0;
    static void authTask(void *context);
    void releaseJob();

  public:
    bool begin();
    bool provisioned() const {
        return valid;
    }
    bool mustChange() const {
        return !verifier.changed && !developer;
    }
    const char *apPassword() const;
    bool startAuth(const String &current, const String &next, const String &confirm, bool change,
                   String &id, String &error);
    bool startPortalAuth(const String &password, String &id, String &error);
    bool authResult(const String &id, bool &pending, bool &changed, bool &developerLogin,
                    String &error);
    void cancelAuth() {
        cancelled = true;
        jobId = "";
    }
    void loop();
    bool authorize(const String &cookie, const String &csrfHeader, uint32_t now, bool mutate) const;
    bool authorizeDeveloper(const String &cookie, const String &csrfHeader, uint32_t now,
                            bool mutate) const;
    bool developerUnlocked() const {
        return developer;
    }
    bool serviceRecover(const String &token, const String &next);
    bool serviceAuthorized(const String &token) const;
    void logout();
    String sessionCookie() const {
        return "cws_session=" + session + "; HttpOnly; SameSite=Strict; Path=/; Max-Age=300";
    }
    const String &csrfToken() const {
        return csrf;
    }
    static String randomHex(size_t bytes);
};
} // namespace cws
