#pragma once
#include <Arduino.h>
namespace wifi_manager {
  void begin(); void update();
  void openConfigurationWindow(uint32_t durationMs);
  bool provisioning(); uint32_t configurationRemainingMs();
  String provisioningSsid(); String provisioningPassword(); String ip(); String status(); int rssi();
}
