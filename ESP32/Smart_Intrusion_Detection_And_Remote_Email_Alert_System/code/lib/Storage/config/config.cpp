#include "config.h"
#include <Preferences.h>
#include <mbedtls/sha256.h>

namespace {
Preferences prefs;
DeviceConfig cfg;

String hashPassword(const String &salt, const String &password) {
  uint8_t digest[32];
  String source = salt + ":" + password;
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);
  mbedtls_sha256_update_ret(&ctx, reinterpret_cast<const unsigned char *>(source.c_str()), source.length());
  mbedtls_sha256_finish_ret(&ctx, digest);
  mbedtls_sha256_free(&ctx);
  char out[65];
  for (size_t i = 0; i < sizeof(digest); ++i) sprintf(out + i * 2, "%02x", digest[i]);
  out[64] = 0;
  return String(out);
}

uint32_t bounded(uint32_t value, uint32_t lo, uint32_t hi) { return value < lo ? lo : (value > hi ? hi : value); }
float boundedFloat(float value, float lo, float hi) { return value < lo ? lo : (value > hi ? hi : value); }
String jsonValue(const String &json, const char *name) {
  String key = String("\"") + name + "\"";
  int p = json.indexOf(key); if (p < 0) return "";
  p = json.indexOf(':', p + key.length()); if (p < 0) return "";
  ++p; while (p < (int)json.length() && isspace(json[p])) ++p;
  if (p >= (int)json.length()) return "";
  if (json[p] == '\"') { int e = json.indexOf('\"', p + 1); return e < 0 ? "" : json.substring(p + 1, e); }
  int e = json.indexOf(',', p); if (e < 0) e = json.indexOf('}', p); return json.substring(p, e < 0 ? json.length() : e);
}
bool boolValue(const String &json, const char *name, bool current) { String v = jsonValue(json, name); return v.length() ? (v == "true" || v == "1") : current; }
uint32_t numberValue(const String &json, const char *name, uint32_t current, uint32_t lo, uint32_t hi) { String v=jsonValue(json,name); return v.length()?bounded(v.toInt(),lo,hi):current; }
float floatValue(const String &json, const char *name, float current, float lo, float hi) { String v=jsonValue(json,name); return v.length()?boundedFloat(v.toFloat(),lo,hi):current; }
void putString(const char *key, const String &v) { prefs.putString(key, v); }
}

namespace config {
void begin() {
  prefs.begin("intrusion", false);
  cfg.deviceName = prefs.getString("name", cfg.deviceName); cfg.location = prefs.getString("location", cfg.location); cfg.timezone = prefs.getString("tz", cfg.timezone);
  cfg.armed = prefs.getBool("armed", cfg.armed); cfg.audibleAlarm = prefs.getBool("audible", cfg.audibleAlarm); cfg.repeatAlarm = prefs.getBool("repeat", cfg.repeatAlarm);
  cfg.pirWarmupMs = prefs.getULong("warmup", cfg.pirWarmupMs); cfg.motionConfirmMs=prefs.getULong("confirm",cfg.motionConfirmMs); cfg.clearConfirmMs=prefs.getULong("clear",cfg.clearConfirmMs); cfg.alarmDurationMs=prefs.getULong("alarm",cfg.alarmDurationMs); cfg.notificationIntervalMs=prefs.getULong("notify",cfg.notificationIntervalMs); cfg.maxRetries=prefs.getUChar("retries",cfg.maxRetries); cfg.configWindowMs=prefs.getULong("configWin",cfg.configWindowMs); cfg.buttonLongPressMs=prefs.getULong("buttonHold",cfg.buttonLongPressMs); cfg.batteryMonitoringEnabled=prefs.getBool("batEnabled",cfg.batteryMonitoringEnabled); cfg.batteryDividerScale=prefs.getFloat("batScale",cfg.batteryDividerScale); cfg.batteryEmptyVoltage=prefs.getFloat("batEmpty",cfg.batteryEmptyVoltage); cfg.batteryFullVoltage=prefs.getFloat("batFull",cfg.batteryFullVoltage); cfg.batteryLowVoltage=prefs.getFloat("batLow",cfg.batteryLowVoltage); cfg.batteryCriticalVoltage=prefs.getFloat("batCrit",cfg.batteryCriticalVoltage);
  cfg.wifiSsid=prefs.getString("wifiSsid",""); cfg.wifiPassword=prefs.getString("wifiPass",""); cfg.smtpHost=prefs.getString("smtpHost",cfg.smtpHost); cfg.smtpPort=prefs.getUShort("smtpPort",cfg.smtpPort); cfg.senderEmail=prefs.getString("sender",""); cfg.senderPassword=prefs.getString("senderPass",""); cfg.recipientEmail=prefs.getString("recipient",""); cfg.senderName=prefs.getString("senderName",cfg.senderName); cfg.subjectTemplate=prefs.getString("subject",cfg.subjectTemplate); cfg.bodyTemplate=prefs.getString("body",cfg.bodyTemplate);
}
DeviceConfig &get() { return cfg; }
bool save() {
  putString("name",cfg.deviceName); putString("location",cfg.location); putString("tz",cfg.timezone); prefs.putBool("armed",cfg.armed); prefs.putBool("audible",cfg.audibleAlarm); prefs.putBool("repeat",cfg.repeatAlarm); prefs.putULong("warmup",cfg.pirWarmupMs); prefs.putULong("confirm",cfg.motionConfirmMs); prefs.putULong("clear",cfg.clearConfirmMs); prefs.putULong("alarm",cfg.alarmDurationMs); prefs.putULong("notify",cfg.notificationIntervalMs); prefs.putUChar("retries",cfg.maxRetries); prefs.putULong("configWin",cfg.configWindowMs); prefs.putULong("buttonHold",cfg.buttonLongPressMs); prefs.putBool("batEnabled",cfg.batteryMonitoringEnabled); prefs.putFloat("batScale",cfg.batteryDividerScale); prefs.putFloat("batEmpty",cfg.batteryEmptyVoltage); prefs.putFloat("batFull",cfg.batteryFullVoltage); prefs.putFloat("batLow",cfg.batteryLowVoltage); prefs.putFloat("batCrit",cfg.batteryCriticalVoltage); putString("wifiSsid",cfg.wifiSsid); putString("wifiPass",cfg.wifiPassword); putString("smtpHost",cfg.smtpHost); prefs.putUShort("smtpPort",cfg.smtpPort); putString("sender",cfg.senderEmail); putString("senderPass",cfg.senderPassword); putString("recipient",cfg.recipientEmail); putString("senderName",cfg.senderName); putString("subject",cfg.subjectTemplate); putString("body",cfg.bodyTemplate); return true;
}
bool updateFromJson(const String &body, String &error) {
  if (body.length() > 4096) { error="Payload too large"; return false; }
  String v;
  #define TEXT_FIELD(field,key,max) v=jsonValue(body,key); if(v.length()){if(v.length()>max){error=String(key)+" is too long";return false;}cfg.field=v;}
  TEXT_FIELD(deviceName,"deviceName",48) TEXT_FIELD(location,"location",64) TEXT_FIELD(timezone,"timezone",40) TEXT_FIELD(wifiSsid,"wifiSsid",32) TEXT_FIELD(wifiPassword,"wifiPassword",64) TEXT_FIELD(smtpHost,"smtpHost",96) TEXT_FIELD(senderEmail,"senderEmail",96) TEXT_FIELD(senderPassword,"senderPassword",96) TEXT_FIELD(recipientEmail,"recipientEmail",160) TEXT_FIELD(senderName,"senderName",64) TEXT_FIELD(subjectTemplate,"subjectTemplate",160) TEXT_FIELD(bodyTemplate,"bodyTemplate",512)
  #undef TEXT_FIELD
  cfg.armed=boolValue(body,"armed",cfg.armed); cfg.audibleAlarm=boolValue(body,"audibleAlarm",cfg.audibleAlarm); cfg.repeatAlarm=boolValue(body,"repeatAlarm",cfg.repeatAlarm); cfg.batteryMonitoringEnabled=boolValue(body,"batteryMonitoringEnabled",cfg.batteryMonitoringEnabled); cfg.pirWarmupMs=numberValue(body,"pirWarmupMs",cfg.pirWarmupMs,0,300000); cfg.motionConfirmMs=numberValue(body,"motionConfirmMs",cfg.motionConfirmMs,20,10000); cfg.clearConfirmMs=numberValue(body,"clearConfirmMs",cfg.clearConfirmMs,100,60000); cfg.alarmDurationMs=numberValue(body,"alarmDurationMs",cfg.alarmDurationMs,1000,600000); cfg.notificationIntervalMs=numberValue(body,"notificationIntervalMs",cfg.notificationIntervalMs,1000,3600000); cfg.configWindowMs=numberValue(body,"configWindowMs",cfg.configWindowMs,30000,600000); cfg.buttonLongPressMs=numberValue(body,"buttonLongPressMs",cfg.buttonLongPressMs,3000,15000); cfg.batteryDividerScale=floatValue(body,"batteryDividerScale",cfg.batteryDividerScale,1.0f,10.0f); cfg.batteryEmptyVoltage=floatValue(body,"batteryEmptyVoltage",cfg.batteryEmptyVoltage,2.0f,4.5f); cfg.batteryFullVoltage=floatValue(body,"batteryFullVoltage",cfg.batteryFullVoltage,cfg.batteryEmptyVoltage+0.1f,5.0f); cfg.batteryLowVoltage=floatValue(body,"batteryLowVoltage",cfg.batteryLowVoltage,2.0f,5.0f); cfg.batteryCriticalVoltage=floatValue(body,"batteryCriticalVoltage",cfg.batteryCriticalVoltage,2.0f,cfg.batteryLowVoltage); cfg.smtpPort=(uint16_t)numberValue(body,"smtpPort",cfg.smtpPort,1,65535); cfg.maxRetries=(uint8_t)numberValue(body,"maxRetries",cfg.maxRetries,0,5); return save();
}
bool configuredWifi() { return cfg.wifiSsid.length() > 0; }
bool hasAdminPassword() { return prefs.isKey("adminHash"); }
bool setAdminPassword(const String &password, String &error) { if(password.length()<12){error="Use at least 12 characters";return false;} String salt=String((uint32_t)esp_random(),HEX)+String((uint32_t)esp_random(),HEX); prefs.putString("adminSalt",salt); prefs.putString("adminHash",hashPassword(salt,password)); return true; }
bool verifyAdminPassword(const String &password) { String salt=prefs.getString("adminSalt",""); String saved=prefs.getString("adminHash",""); return salt.length() && saved.length() && hashPassword(salt,password)==saved; }
void factoryReset() { prefs.clear(); ESP.restart(); }
}
