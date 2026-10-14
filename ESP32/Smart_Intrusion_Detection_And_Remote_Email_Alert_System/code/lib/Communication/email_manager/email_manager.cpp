#include "email_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "Storage/config/config.h"
#include "Communication/wifi_manager/wifi_manager.h"
namespace
{
  struct N
  {
    uint32_t id, cycle;
    time_t eventEpoch;
    bool test;
    uint8_t tries;
  };
  QueueHandle_t q;
  String last = "not configured";
  uint32_t lastQueued = 0;
  const char ROOT[] PROGMEM = R"PEM(-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----)PEM";
  String enc(const String &s)
  {
    const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String o;
    for (unsigned i = 0; i < s.length(); i += 3)
    {
      uint32_t n = (uint8_t)s[i] << 16;
      if (i + 1 < s.length())
        n |= (uint8_t)s[i + 1] << 8;
      if (i + 2 < s.length())
        n |= (uint8_t)s[i + 2];
      o += t[n >> 18];
      o += t[(n >> 12) & 63];
      o += i + 1 < s.length() ? t[(n >> 6) & 63] : '=';
      o += i + 2 < s.length() ? t[n & 63] : '=';
    }
    return o;
  }
  String smtpBody(const String &body)
  {
    String out;
    bool lineStart = true;
    for (size_t i = 0; i < body.length(); ++i)
    {
      char ch = body[i];
      if (ch == '\r') continue;
      if (ch == '\n') { out += "\r\n"; lineStart = true; continue; }
      if (lineStart && ch == '.') out += '.'; // SMTP dot-stuffing
      out += ch;
      lineStart = false;
    }
    return out;
  }
  String incidentDetails(const DeviceConfig &config, const N &notification)
  {
    String details = "\n\nLocation: " + config.location;
    if (!notification.test) details += "\nEvent ID: " + String(notification.id);
    details += "\nTime: " + wifi_manager::formatTime(notification.eventEpoch);
    return details;
  }
  bool reply(WiFiClientSecure &c, int wanted = 2)
  {
    uint32_t until = millis() + 12000;
    String r;
    while ((int32_t)(until - millis()) > 0)
    {
      while (c.available())
      {
        char x = c.read();
        r += x;
        if (x == '\n')
        {
          if (r.length() >= 3 && r[0] - '0' == wanted)
            return true;
          if (r.length() < 4 || r[3] != '-')
          {
            last = "SMTP: " + r;
            return false;
          }
          r = "";
        }
      }
      vTaskDelay(1);
    }
    last = "SMTP timeout";
    return false;
  }
  bool send(N n)
  {
    // Keep this delivery internally consistent if an administrator saves new
    // settings while the SMTP task is in progress.
    DeviceConfig x = config::get();
    if (WiFi.status() != WL_CONNECTED || !x.senderEmail.length() || !x.senderPassword.length() || !x.recipientEmails.length())
    {
      last = "waiting for Wi-Fi or Gmail setup";
      return false;
    }
    WiFiClientSecure c;
    c.setCACert(ROOT);
    if (!c.connect(x.smtpHost.c_str(), x.smtpPort) || !reply(c))
    {
      last = "TLS verification/connect failed";
      return false;
    }
    auto cmd = [&](String z, int code = 2)
    {c.print(z+"\r\n");return reply(c,code); };
    if (!cmd("EHLO esp32") || !cmd("AUTH LOGIN", 3) || !cmd(enc(x.senderEmail), 3) || !cmd(enc(x.senderPassword)) || !cmd("MAIL FROM:<" + x.senderEmail + ">"))
      return false;
    // Recipients are configured as a canonical comma-separated list.  One
    // SMTP envelope preserves a single event/retry lifecycle, while the
    // undisclosed To header avoids exposing the recipient list to everyone.
    int from = 0;
    uint8_t recipientCount = 0;
    while (from <= (int)x.recipientEmails.length()) {
      int comma = x.recipientEmails.indexOf(',', from);
      String recipient = x.recipientEmails.substring(from, comma < 0 ? x.recipientEmails.length() : comma);
      recipient.trim();
      if (recipient.length()) {
        if (!cmd("RCPT TO:<" + recipient + ">")) return false;
        ++recipientCount;
      }
      if (comma < 0) break;
      from = comma + 1;
    }
    if (!recipientCount || !cmd("DATA", 3)) return false;
    String body = n.test ? "This is a test email from " + x.deviceName : x.bodyTemplate;
    body += incidentDetails(x, n);
    body = smtpBody(body);
    String subject = n.test ? "Intrusion detector test - " + x.deviceName : "SECURITY ALERT - " + x.deviceName;
    subject.replace("\r", " "); subject.replace("\n", " ");
    String senderName = x.senderName;
    senderName.replace("\r", " "); senderName.replace("\n", " "); senderName.replace("\"", "'");
    bool ok = cmd("From: \"" + senderName + "\" <" + x.senderEmail + ">\r\nTo: undisclosed-recipients:;\r\nSubject: " + subject + "\r\nMIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\n\r\n" + body + "\r\n.");
    last = ok ? "SMTP accepted notification" : last;
    c.stop();
    return ok;
  }
  void task(void *)
  {
    N n;
    for (;;)
      if (xQueueReceive(q, &n, portMAX_DELAY))
      {
        while (!send(n))
        {
          // Keep the original event timestamp with this item while
          // connectivity is unavailable. Retry promptly at first, then at a
          // lower rate without discarding the pending notification.
          uint32_t waitMs = n.tries++ < config::get().maxRetries ? 3000 : 60000;
          if (n.tries > config::get().maxRetries)
            last = "delivery pending; retrying";
          vTaskDelay(waitMs / portTICK_PERIOD_MS);
        }
      }
  }
}
namespace email_manager
{
  void begin()
  {
    q = xQueueCreate(8, sizeof(N));
    xTaskCreatePinnedToCore(task, "smtp", 8192, nullptr, 1, nullptr, 0);
  }
  bool queue(uint32_t id, uint32_t c, bool test)
  {
    if (!test && millis() - lastQueued < config::get().notificationIntervalMs)
    {
      last = "notification rate limited";
      return false;
    }
    N n{id, c, wifi_manager::currentEpoch(), test, 0};
    if (xQueueSendToBack(q, &n, 0) != pdTRUE)
    {
      last = "notification queue full";
      return false;
    }
    if (!test)
      lastQueued = millis();
    last = "notification queued";
    return true;
  }
  bool queueIncident(uint32_t id, uint32_t c, time_t eventEpoch)
  {
    if (millis() - lastQueued < config::get().notificationIntervalMs)
    {
      last = "notification rate limited";
      return false;
    }
    N n{id, c, eventEpoch, false, 0};
    if (xQueueSendToBack(q, &n, 0) != pdTRUE)
    {
      last = "notification queue full";
      return false;
    }
    lastQueued = millis();
    last = "notification queued";
    return true;
  }
  String status() { return last; }
  uint8_t pending() { return q ? uxQueueMessagesWaiting(q) : 0; }
}
