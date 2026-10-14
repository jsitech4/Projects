#include "wifi_manager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <time.h>
#include "Storage/config/config.h"
namespace
{
  bool ap = false, mdns = false, firstSetup = false, timeRequested = false;
  uint32_t lastAttempt = 0, windowEnds = 0;
  String ssid, pass, appliedTimezone;
  void startAp(bool initial = false)
  {
    if (ap)
      return;
    uint64_t id = ESP.getEfuseMac();
    ssid = "IntrusionDetector-" + String((uint16_t)id, HEX);
    pass = "ID-" + String((uint32_t)(id >> 16), HEX) + "!";
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), pass.c_str());
    ap = true;
    firstSetup = initial;
    Serial.printf("Config AP %s password %s at %s\n", ssid.c_str(), pass.c_str(), WiFi.softAPIP().toString().c_str());
  }
  void connect()
  {
    DeviceConfig &c = config::get();
    if (!c.wifiSsid.length())
    {
      startAp(true);
      return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(c.wifiSsid.c_str(), c.wifiPassword.c_str());
    lastAttempt = millis();
  }
  void requestTime()
  {
    String timezone = config::get().timezone;
    timezone.trim();
    if (!timezone.length())
      timezone = "WAT-1";
    configTzTime(timezone.c_str(), "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    appliedTimezone = timezone;
    timeRequested = true;
  }
}
namespace
{
  time_t trustedEpoch = 0;
  uint32_t trustedMillis = 0;
  void rememberTrustedTime()
  {
    time_t now = time(nullptr);
    if (now >= 1704067200)
    {
      trustedEpoch = now;
      trustedMillis = millis();
    }
  }
}
namespace
{
  bool internetReachable = false;
  uint32_t lastInternetProbe = 0;
  void probeInternet()
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      internetReachable = false;
      return;
    }
    if (lastInternetProbe && millis() - lastInternetProbe < 30000)
      return;
    lastInternetProbe = millis();
    WiFiClient probe;
    probe.setTimeout(250);
    internetReachable = probe.connect(IPAddress(1, 1, 1, 1), 53, 250);
    probe.stop();
  }
}
namespace wifi_manager
{
  void begin()
  {
    WiFi.persistent(false);
    if (config::configuredWifi())
      connect();
    else
      startAp(true);
  }
  void update()
  {
    if (ap && !firstSetup && windowEnds && (int32_t)(millis() - windowEnds) >= 0)
    {
      WiFi.softAPdisconnect(true);
      ap = false;
      windowEnds = 0;
      WiFi.mode(WIFI_STA);
      connect();
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      if (!timeRequested || appliedTimezone != config::get().timezone)
        requestTime();
      rememberTrustedTime();
      probeInternet();
      if (!mdns)
      {
        mdns = MDNS.begin("intrusion-detector");
      }
      return;
    }
    internetReachable = false;
    if (config::configuredWifi() && millis() - lastAttempt > 15000)
      connect();
  }
  void openConfigurationWindow(uint32_t durationMs)
  {
    if (!config::configuredWifi())
      return;
    startAp(false);
    windowEnds = millis() + durationMs;
    Serial.printf("Configuration window active for %lu seconds\n", durationMs / 1000);
  }
  bool provisioning() { return ap; }
  uint32_t configurationRemainingMs()
  {
    if (!ap || firstSetup || !windowEnds)
      return 0;
    return (int32_t)(windowEnds - millis()) > 0 ? windowEnds - millis() : 0;
  }
  String provisioningSsid() { return ssid; }
  String provisioningPassword() { return pass; }
  String ip() { return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : (ap ? WiFi.softAPIP().toString() : String("unavailable")); }
  String status() { return WiFi.status() == WL_CONNECTED ? (ap ? "connected + config AP" : "connected") : (ap ? "provisioning" : "connecting"); }
  int rssi() { return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0; }
  bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }
  bool internetAvailable() { return internetReachable; }
  bool timeSynchronized() { return currentEpoch() >= 1704067200; }
  time_t currentEpoch()
  {
    if (!trustedEpoch)
      return 0;
    return trustedEpoch + (uint32_t)(millis() - trustedMillis) / 1000;
  }
  String formatTime(time_t epoch)
  {
    if (epoch < 1704067200)
      return "Time unavailable";
    tm local{};
    if (!localtime_r(&epoch, &local))
      return "Time unavailable";
    char text[36];
    return strftime(text, sizeof(text), "%d %b %Y, %H:%M:%S %Z", &local) ? String(text) : String("Time unavailable");
  }
  String localTime() { return formatTime(currentEpoch()); }
}
