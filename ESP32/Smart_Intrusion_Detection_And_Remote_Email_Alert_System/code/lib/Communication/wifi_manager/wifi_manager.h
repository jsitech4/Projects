#pragma once
#include <Arduino.h>
#include <time.h>
namespace wifi_manager {
  void begin(); void update();
  void openConfigurationWindow(uint32_t durationMs);
  bool provisioning(); uint32_t configurationRemainingMs();
  String provisioningSsid(); String provisioningPassword(); String ip(); String status(); int rssi();
  bool wifiConnected(); bool internetAvailable();
  bool timeSynchronized(); time_t currentEpoch(); String formatTime(time_t epoch); String localTime();
}
