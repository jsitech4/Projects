#include "wifi_manager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Storage/config/config.h"
namespace { bool ap=false,mdns=false,firstSetup=false,timeRequested=false; uint32_t lastAttempt=0,windowEnds=0; String ssid,pass; void startAp(bool initial=false){if(ap)return; uint64_t id=ESP.getEfuseMac();ssid="IntrusionDetector-"+String((uint16_t)id,HEX);pass="ID-"+String((uint32_t)(id>>16),HEX)+"!";WiFi.mode(WIFI_AP_STA);WiFi.softAP(ssid.c_str(),pass.c_str());ap=true;firstSetup=initial;Serial.printf("Config AP %s password %s at %s\n",ssid.c_str(),pass.c_str(),WiFi.softAPIP().toString().c_str());} void connect(){DeviceConfig &c=config::get();if(!c.wifiSsid.length()) {startAp(true);return;}WiFi.mode(WIFI_STA);WiFi.begin(c.wifiSsid.c_str(),c.wifiPassword.c_str());lastAttempt=millis();} }
namespace wifi_manager {
void begin(){WiFi.persistent(false);if(config::configuredWifi())connect();else startAp(true);}
void update(){if(ap&&!firstSetup&&windowEnds&&(int32_t)(millis()-windowEnds)>=0){WiFi.softAPdisconnect(true);ap=false;windowEnds=0;WiFi.mode(WIFI_STA);connect();}if(WiFi.status()==WL_CONNECTED){if(!timeRequested){configTime(3600,0,"pool.ntp.org","time.google.com");timeRequested=true;}if(!mdns){mdns=MDNS.begin("intrusion-detector");}return;}if(config::configuredWifi() && millis()-lastAttempt>15000)connect();}
void openConfigurationWindow(uint32_t durationMs){if(!config::configuredWifi())return;startAp(false);windowEnds=millis()+durationMs;Serial.printf("Configuration window active for %lu seconds\n",durationMs/1000);}
bool provisioning(){return ap;} uint32_t configurationRemainingMs(){if(!ap||firstSetup||!windowEnds)return 0;return (int32_t)(windowEnds-millis())>0?windowEnds-millis():0;} String provisioningSsid(){return ssid;} String provisioningPassword(){return pass;} String ip(){return WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():(ap?WiFi.softAPIP().toString():String("unavailable"));} String status(){return WiFi.status()==WL_CONNECTED?(ap?"connected + config AP":"connected"):(ap?"provisioning":"connecting");} int rssi(){return WiFi.status()==WL_CONNECTED?WiFi.RSSI():0;}
}
