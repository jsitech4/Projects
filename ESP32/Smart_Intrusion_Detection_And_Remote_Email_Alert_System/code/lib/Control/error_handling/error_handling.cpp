#include "error_handling/error_handling.h"
#include "esp_system.h"

namespace error_handling
{

  bool watchdogError = false;
  bool codeError = false;

  void begin()
  {
    esp_reset_reason_t reason = esp_reset_reason();
    watchdogError = (reason == ESP_RST_WDT || reason == ESP_RST_TASK_WDT);
    codeError = false;
  }

  void update()
  {
    // nothing active here
  }

  void setCodeError(bool state)
  {
    codeError = state;
  }

  bool hasError()
  {
    return watchdogError || codeError;
  }

}
