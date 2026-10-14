#include "event_log.h"
#include <Preferences.h>
#include "Communication/wifi_manager/wifi_manager.h"
namespace
{
  Preferences p;
  const uint8_t Capacity = 8;
  uint8_t writeIndex = 0, count = 0;
  String esc(String s)
  {
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    return s;
  }
}
namespace event_log
{
  void begin()
  {
    p.begin("eventlog", false);
    writeIndex = p.getUChar("idx", 0);
    count = p.getUChar("count", 0);
  }
  void record(const IntrusionStatus &s, bool closed)
  {
    String v = String("{\"id\":") + s.eventId + ",\"uptime_ms\":" + millis() + ",\"event_time\":\"" + esc(wifi_manager::formatTime(s.eventEpoch)) + "\",\"cycles\":" + s.alarmCycle + ",\"state\":\"" + (closed ? "closed" : "active") + "\"}";
    p.putString((String("e") + writeIndex).c_str(), v);
    writeIndex = (writeIndex + 1) % Capacity;
    if (count < Capacity)
      count++;
    p.putUChar("idx", writeIndex);
    p.putUChar("count", count);
  }
  String json()
  {
    String result = "[";
    for (uint8_t n = 0; n < count; n++)
    {
      uint8_t slot = (writeIndex + Capacity - 1 - n) % Capacity;
      String v = p.getString((String("e") + slot).c_str(), "");
      if (!v.length())
        continue;
      if (result.length() > 1)
        result += ',';
      result += v;
    }
    return result + "]";
  }
  void clear()
  {
    p.clear();
    writeIndex = 0;
    count = 0;
  }
}
