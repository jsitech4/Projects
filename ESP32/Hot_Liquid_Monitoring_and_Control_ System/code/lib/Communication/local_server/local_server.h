#pragma once

#include <Arduino.h>

namespace local_server
{
  void begin();
  void update();

  bool isRunning();

  String getIp();
  String getSsid();
}
