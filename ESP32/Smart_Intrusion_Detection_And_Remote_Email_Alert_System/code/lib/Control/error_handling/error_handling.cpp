#include "error_handling/error_handling.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

namespace error_handling
{

  bool watchdogError = false;
  bool codeError = false;
  bool taskWatchdogEnabled = false;

  void begin()
  {
    esp_reset_reason_t reason = esp_reset_reason();
    watchdogError = (reason == ESP_RST_WDT || reason == ESP_RST_TASK_WDT);
    codeError = false;
  }

  void enableWatchdog(uint8_t timeoutSeconds)
  {
    if (timeoutSeconds == 0) { codeError = true; return; }
    esp_err_t initialized = esp_task_wdt_init(timeoutSeconds, true);
    if (initialized != ESP_OK && initialized != ESP_ERR_INVALID_STATE) { codeError = true; return; }
    esp_err_t added = esp_task_wdt_add(nullptr);
    if (added == ESP_OK || added == ESP_ERR_INVALID_ARG) {
      taskWatchdogEnabled = true;
      return;
    }
    codeError = true;
  }

  void update()
  {
    if (taskWatchdogEnabled && esp_task_wdt_reset() != ESP_OK) {
      taskWatchdogEnabled = false;
      codeError = true;
    }
  }

  bool watchdogEnabled() { return taskWatchdogEnabled; }

  void setCodeError(bool state)
  {
    codeError = state;
  }

  bool hasError()
  {
    return watchdogError || codeError;
  }

}
