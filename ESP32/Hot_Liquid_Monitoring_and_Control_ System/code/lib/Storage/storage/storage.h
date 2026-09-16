#pragma once

#include <Arduino.h>
#include <FS.h>

namespace storage
{
  void begin();
  void update();

  void logNow();

  void logEvent(const String &event,
                const String &message);

  bool isReady();
  bool isSdReady();
  bool isInternalReady();

  String getBackendName();

  void setLogInterval(unsigned long intervalMs);

  const char *getFileName();
  const char *getLiquidLogFileName();
  const char *getAnalysisLogFileName();
  const char *getEventLogFileName();

  String readFile(const char *path);

  String readTail(const char *path,
                  size_t maxBytes);

  size_t getFileSize(const char *path);

  File openRead(const char *path);

  // =========================================================
  // Persistent system settings
  // =========================================================

  bool loadTankSettings(
      float &fullDistanceCm,
      float &lowDistanceCm,
      float &fullLevelPercent,
      float &lowLevelPercent,
      uint8_t &tankLatchMode,
      float &lowTemperatureC,
      float &highTemperatureC,
      uint8_t &temperatureLatchMode);

  bool saveTankSettings(
      float fullDistanceCm,
      float lowDistanceCm,
      float fullLevelPercent,
      float lowLevelPercent,
      uint8_t tankLatchMode,
      float lowTemperatureC,
      float highTemperatureC,
      uint8_t temperatureLatchMode);
}
