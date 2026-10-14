#pragma once

#include <Arduino.h>

struct DeviceConfig {
  String deviceName = "Intrusion Detector";
  String location = "Unconfigured";
  String timezone = "WAT-1";
  bool armed = true;
  bool audibleAlarm = true;
  bool repeatAlarm = true;
  uint32_t pirWarmupMs = 30000;
  uint32_t motionConfirmMs = 150;
  uint32_t clearConfirmMs = 2000;
  uint32_t alarmDurationMs = 20000;
  uint32_t notificationIntervalMs = 20000;
  uint8_t maxRetries = 2;
  uint32_t configWindowMs = 120000;
  uint32_t buttonLongPressMs = 5000;
  bool batteryMonitoringEnabled = true;
  float batteryDividerScale = 1.61f;
  float batteryEmptyVoltage = 3.00f;
  float batteryFullVoltage = 4.20f;
  float batteryLowVoltage = 3.30f;
  float batteryCriticalVoltage = 3.10f;
  String wifiSsid;
  String wifiPassword;
  String smtpHost = "smtp.gmail.com";
  uint16_t smtpPort = 465;
  String senderEmail;
  String senderPassword;
  // Comma-separated canonical list.  Kept small so it remains practical for
  // the ESP32's NVS and for a single SMTP transaction.
  String recipientEmails;
  String senderName = "ESP32 Security";
  String subjectTemplate = "SECURITY ALERT";
  String bodyTemplate = "Intrusion detected.";
};

namespace config {
  void begin();
  DeviceConfig &get();
  bool save();
  bool updateFromJson(const String &body, String &error);
  bool configuredWifi();
  bool hasAdminPassword();
  bool setAdminPassword(const String &password, String &error);
  bool verifyAdminPassword(const String &password);
  void factoryReset();
}
