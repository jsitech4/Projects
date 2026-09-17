#include <Arduino.h>
#include <LittleFS.h>
#include <FS.h>
#include <math.h>

#include "storage.h"

#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "load_relay/load_relay.h"
#include "device_manager/device_manager.h"

namespace storage
{
  // =========================================================
  // FILE SYSTEM
  // =========================================================

  static fs::FS *activeFS = nullptr;

  static bool ready = false;
  static bool internalReady = false;

  // =========================================================
  // LOGGING
  // =========================================================

  static unsigned long lastLog = 0;

  static unsigned long logInterval = 5000;

  static const char *liquidLogFile =
      "/liquid_log.csv";

  static const char *analysisLogFile =
      "/analysis_log.csv";

  static const char *eventLogFile =
      "/event_log.csv";

  // =========================================================
  // SETTINGS
  // =========================================================

  static const char *tankSettingsFile =
      "/tank_settings.cfg";

  static const char *backendName =
      "INTERNAL FLASH";

  // =========================================================
  // HELPERS
  // =========================================================

  static bool hasActiveStorage()
  {
    return ready &&
           activeFS != nullptr;
  }

  static bool fileExists(const char *path)
  {
    if (!hasActiveStorage())
    {
      return false;
    }

    return activeFS->exists(path);
  }

  static String cleanCsvText(String text)
  {
    text.replace("\r", " ");
    text.replace("\n", " ");
    text.replace(",", ";");

    return text;
  }

  static bool appendLine(
      const char *path,
      const String &line)
  {
    if (!hasActiveStorage())
    {
      return false;
    }

    File file =
        activeFS->open(path, FILE_APPEND);

    if (!file)
    {
      file =
          activeFS->open(path, FILE_WRITE);
    }

    if (!file)
    {
      return false;
    }

    file.println(line);
    file.close();

    return true;
  }

  static void createFileWithHeader(
      const char *path,
      const String &header)
  {
    if (!hasActiveStorage())
    {
      return;
    }

    if (fileExists(path))
    {
      return;
    }

    File file =
        activeFS->open(path, FILE_WRITE);

    if (!file)
    {
      return;
    }

    file.println(header);
    file.close();
  }

  static void createHeaders()
  {
    createFileWithHeader(
        liquidLogFile,
        "millis,temp_c,temp_valid,"
        "distance_cm,level_percent,"
        "relay_on,relay_requested");

    createFileWithHeader(
        analysisLogFile,
        "millis,temperature_c,"
        "level_percent,distance_cm");

    createFileWithHeader(
        eventLogFile,
        "millis,event,message");
  }

  // =========================================================
  // BEGIN
  // =========================================================

  void begin()
  {
    ready = false;
    internalReady = false;
    activeFS = nullptr;

    internalReady =
        LittleFS.begin(true);

    if (!internalReady)
    {
      return;
    }

    activeFS = &LittleFS;

    ready = true;

    createHeaders();

    logEvent(
        "BOOT",
        "Internal flash storage initialized");
  }

  // =========================================================
  // UPDATE
  // =========================================================

  void update()
  {
    if (!ready)
    {
      return;
    }

    unsigned long now = millis();

    if ((unsigned long)(now - lastLog) >=
        logInterval)
    {
      lastLog = now;

      logNow();
    }
  }

  // =========================================================
  // LOG NOW
  // =========================================================

  void logNow()
  {
    if (!ready)
    {
      return;
    }

    device_manager::Snapshot snap =
        device_manager::getSnapshot();

    // -----------------------------------------------------
    // Liquid log
    // -----------------------------------------------------

    String liquidLine;

    liquidLine.reserve(180);

    liquidLine += String(millis());
    liquidLine += ",";

    if (snap.tempValid)
    {
      liquidLine +=
          String(snap.temperatureC, 2);
    }
    else
    {
      liquidLine += "nan";
    }

    liquidLine += ",";

    liquidLine +=
        snap.tempValid ? "1" : "0";

    liquidLine += ",";

    if (snap.levelValid)
    {
      liquidLine +=
          String(snap.distanceCm, 3);
    }
    else
    {
      liquidLine += "nan";
    }

    liquidLine += ",";

    if (snap.levelValid)
    {
      liquidLine +=
          String(snap.levelPercent, 1);
    }
    else
    {
      liquidLine += "nan";
    }

    liquidLine += ",";

    liquidLine +=
        snap.relayOn ? "1" : "0";

    liquidLine += ",";

    liquidLine +=
        snap.relayRequested ? "1" : "0";

    appendLine(
        liquidLogFile,
        liquidLine);

    // -----------------------------------------------------
    // Analysis log
    // -----------------------------------------------------

    String analysisLine;

    analysisLine.reserve(120);

    analysisLine += String(millis());
    analysisLine += ",";

    if (snap.tempValid)
    {
      analysisLine +=
          String(snap.temperatureC, 2);
    }
    else
    {
      analysisLine += "nan";
    }

    analysisLine += ",";

    if (snap.levelValid)
    {
      analysisLine +=
          String(snap.levelPercent, 1);
    }
    else
    {
      analysisLine += "nan";
    }

    analysisLine += ",";

    if (snap.levelValid)
    {
      analysisLine +=
          String(snap.distanceCm, 1);
    }
    else
    {
      analysisLine += "nan";
    }

    appendLine(
        analysisLogFile,
        analysisLine);
  }

  // =========================================================
  // EVENT LOG
  // =========================================================

  void logEvent(
      const String &event,
      const String &message)
  {
    if (!ready)
    {
      return;
    }

    String line;

    line.reserve(
        event.length() +
        message.length() +
        30);

    line += String(millis());
    line += ",";

    line += cleanCsvText(event);
    line += ",";

    line += cleanCsvText(message);

    appendLine(
        eventLogFile,
        line);
  }

  // =========================================================
  // STATUS
  // =========================================================

  bool isReady()
  {
    return ready;
  }

  bool isSdReady()
  {
    return false;
  }

  bool isInternalReady()
  {
    return internalReady;
  }

  String getBackendName()
  {
    return String(backendName);
  }

  void setLogInterval(
      unsigned long intervalMs)
  {
    if (intervalMs >= 1000)
    {
      logInterval = intervalMs;
    }
  }

  // =========================================================
  // FILE NAMES
  // =========================================================

  const char *getFileName()
  {
    return liquidLogFile;
  }

  const char *getLiquidLogFileName()
  {
    return liquidLogFile;
  }

  const char *getAnalysisLogFileName()
  {
    return analysisLogFile;
  }

  const char *getEventLogFileName()
  {
    return eventLogFile;
  }

  // =========================================================
  // READ FILE
  // =========================================================

  String readFile(const char *path)
  {
    if (!hasActiveStorage())
    {
      return "";
    }

    File file =
        activeFS->open(path, FILE_READ);

    if (!file)
    {
      return "";
    }

    String content;

    size_t count = 0;

    while (file.available())
    {
      content += char(file.read());

      count++;

      if ((count % 256) == 0)
      {
        yield();
      }
    }

    file.close();

    return content;
  }

  // =========================================================
  // READ TAIL
  // =========================================================

  String readTail(
      const char *path,
      size_t maxBytes)
  {
    if (!hasActiveStorage())
    {
      return "";
    }

    File file =
        activeFS->open(path, FILE_READ);

    if (!file)
    {
      return "";
    }

    size_t size = file.size();

    if (size > maxBytes)
    {
      file.seek(size - maxBytes);
    }

    String content;

    size_t count = 0;

    while (file.available())
    {
      content += char(file.read());

      count++;

      if ((count % 256) == 0)
      {
        yield();
      }
    }

    file.close();

    if (size > maxBytes)
    {
      int firstNewLine =
          content.indexOf('\n');

      if (firstNewLine >= 0)
      {
        content =
            content.substring(
                firstNewLine + 1);
      }
    }

    return content;
  }

  // =========================================================
  // FILE SIZE
  // =========================================================

  size_t getFileSize(const char *path)
  {
    if (!hasActiveStorage())
    {
      return 0;
    }

    File file =
        activeFS->open(path, FILE_READ);

    if (!file)
    {
      return 0;
    }

    size_t size = file.size();

    file.close();

    return size;
  }

  // =========================================================
  // OPEN FILE
  // =========================================================

  File openRead(const char *path)
  {
    if (!hasActiveStorage())
    {
      return File();
    }

    return activeFS->open(
        path,
        FILE_READ);
  }

  // =========================================================
  // LOAD SETTINGS
  // =========================================================
  //
  // Current format:
  //
  // fullDistance,
  // lowDistance,
  // tankLatchMode,
  // lowTemperature,
  // highTemperature,
  // temperatureLatchMode
  //
  // Legacy 3-value files are also accepted:
  //
  // fullPercent,lowPercent,latchMode
  //
  // Legacy files are converted to the current distance-based
  // system using default distance and temperature settings.
  // =========================================================

  bool loadTankSettings(
      float &fullDistance,
      float &lowDistance,
      uint8_t &tankLatch,
      float &lowTemperature,
      float &highTemperature,
      uint8_t &temperatureLatch)
  {
    if (!hasActiveStorage())
    {
      return false;
    }

    if (!activeFS->exists(tankSettingsFile))
    {
      return false;
    }

    File file =
        activeFS->open(
            tankSettingsFile,
            FILE_READ);

    if (!file)
    {
      return false;
    }

    String line =
        file.readStringUntil('\n');

    file.close();

    line.trim();

    if (line.length() == 0)
    {
      return false;
    }

    // -----------------------------------------------------
    // Parse comma-separated values
    // -----------------------------------------------------

    float values[8];

    int valueCount = 0;
    int start = 0;

    while (valueCount < 8)
    {
      int comma =
          line.indexOf(',', start);

      String token;

      if (comma < 0)
      {
        token =
            line.substring(start);
      }
      else
      {
        token =
            line.substring(
                start,
                comma);
      }

      token.trim();

      if (token.length() == 0)
      {
        return false;
      }

      values[valueCount++] =
          token.toFloat();

      if (comma < 0)
      {
        break;
      }

      start = comma + 1;
    }

    // -----------------------------------------------------
    // Current 6-value settings format
    // -----------------------------------------------------

    if (valueCount >= 6)
    {
      float savedFullDistance =
          values[0];

      float savedLowDistance =
          values[1];

      int savedTankLatch =
          static_cast<int>(values[2]);

      float savedLowTemperature =
          values[3];

      float savedHighTemperature =
          values[4];

      int savedTemperatureLatch =
          static_cast<int>(values[5]);

      // ---------------------------------------------------
      // Validate
      // ---------------------------------------------------

      if (!isfinite(savedFullDistance) ||
          !isfinite(savedLowDistance) ||
          !isfinite(savedLowTemperature) ||
          !isfinite(savedHighTemperature))
      {
        return false;
      }

      if (savedFullDistance <= 0.0f ||
          savedLowDistance <= savedFullDistance ||
          savedLowDistance > 500.0f)
      {
        return false;
      }

      if (savedTankLatch < 0 ||
          savedTankLatch > 2)
      {
        return false;
      }

      if (savedLowTemperature >=
          savedHighTemperature)
      {
        return false;
      }

      if (savedLowTemperature < -200.0f ||
          savedHighTemperature > 850.0f)
      {
        return false;
      }

      if (savedTemperatureLatch < 0 ||
          savedTemperatureLatch > 2)
      {
        return false;
      }

      // ---------------------------------------------------
      // Apply loaded values
      // ---------------------------------------------------

      fullDistance =
          savedFullDistance;

      lowDistance =
          savedLowDistance;

      tankLatch =
          static_cast<uint8_t>(
              savedTankLatch);

      lowTemperature =
          savedLowTemperature;

      highTemperature =
          savedHighTemperature;

      temperatureLatch =
          static_cast<uint8_t>(
              savedTemperatureLatch);

      return true;
    }

    // -----------------------------------------------------
    // Legacy 3-value format
    //
    // Old:
    // fullPercent,lowPercent,latchMode
    //
    // The old percentage thresholds are no longer stored.
    // We therefore convert the old file to the default
    // distance calibration.
    // -----------------------------------------------------

    if (valueCount >= 3)
    {
      float savedFullPercent =
          values[0];

      float savedLowPercent =
          values[1];

      int savedLatch =
          static_cast<int>(values[2]);

      if (!isfinite(savedFullPercent) ||
          !isfinite(savedLowPercent))
      {
        return false;
      }

      if (savedFullPercent <= savedLowPercent ||
          savedFullPercent > 100.0f ||
          savedLowPercent < 0.0f ||
          savedLatch < 0 ||
          savedLatch > 2)
      {
        return false;
      }

      // ---------------------------------------------------
      // Convert legacy percentage configuration.
      //
      // The new system derives percentage from distance.
      // Use the existing/default distance calibration.
      // ---------------------------------------------------

      fullDistance = 5.0f;
      lowDistance = 40.0f;

      tankLatch =
          static_cast<uint8_t>(
              savedLatch);

      lowTemperature = 30.0f;
      highTemperature = 80.0f;

      temperatureLatch = 0;

      return true;
    }

    return false;
  }

  // =========================================================
  // SAVE SETTINGS
  // =========================================================
  //
  // Current format:
  //
  // fullDistance,
  // lowDistance,
  // tankLatchMode,
  // lowTemperature,
  // highTemperature,
  // temperatureLatchMode
  // =========================================================

  bool saveTankSettings(
      float fullDistance,
      float lowDistance,
      uint8_t tankLatch,
      float lowTemperature,
      float highTemperature,
      uint8_t temperatureLatch)
  {
    if (!hasActiveStorage())
    {
      return false;
    }

    // -----------------------------------------------------
    // Validate numeric values
    // -----------------------------------------------------

    if (!isfinite(fullDistance) ||
        !isfinite(lowDistance) ||
        !isfinite(lowTemperature) ||
        !isfinite(highTemperature))
    {
      return false;
    }

    // -----------------------------------------------------
    // Validate tank distance calibration
    // -----------------------------------------------------

    if (fullDistance <= 0.0f ||
        lowDistance <= fullDistance ||
        lowDistance > 500.0f)
    {
      return false;
    }

    // -----------------------------------------------------
    // Validate tank latch mode
    // -----------------------------------------------------

    if (tankLatch > 2)
    {
      return false;
    }

    // -----------------------------------------------------
    // Validate temperature thresholds
    // -----------------------------------------------------

    if (lowTemperature >= highTemperature)
    {
      return false;
    }

    if (lowTemperature < -200.0f ||
        highTemperature > 850.0f)
    {
      return false;
    }

    // -----------------------------------------------------
    // Validate temperature latch mode
    // -----------------------------------------------------

    if (temperatureLatch > 2)
    {
      return false;
    }

    // -----------------------------------------------------
    // Open settings file
    // -----------------------------------------------------

    File file =
        activeFS->open(
            tankSettingsFile,
            FILE_WRITE);

    if (!file)
    {
      return false;
    }

    // -----------------------------------------------------
    // Write current 6-value format
    // -----------------------------------------------------

    file.print(fullDistance, 2);
    file.print(',');

    file.print(lowDistance, 2);
    file.print(',');

    file.print(tankLatch);
    file.print(',');

    file.print(lowTemperature, 2);
    file.print(',');

    file.print(highTemperature, 2);
    file.print(',');

    file.println(temperatureLatch);

    file.close();

    return true;
  }

}
