# Configuration trial and recovery

Connection candidates live in RAM until their selected trial passes. Saved configuration is retained throughout the trial. Wi-Fi and MQTT tests are independent: a valid router can be saved even while the backend is unavailable. That state remains explicitly pending backend validation.

The owner selects test-only or test-and-save. Test-only restores saved values after either result. Failed tests, cancelled tests and reset during an uncommitted trial retain the previous NVS revision. A successful saved trial increments revision. A storage commit failure is reported as failure and restores the previous connection settings.

Only one trial runs at a time. Local and remote configuration changes require the expected revision. Developer calibration commits are blocked during a connection trial. `config.patch` rejects Wi-Fi, MQTT and calibration fields; the dedicated connection trials and local developer page own those settings.

At every reboot the owner has a new five-minute portal window, including when saved Wi-Fi is wrong or the backend is unreachable. AP credentials and administrator replacement remain independent of connectivity. A setup session ending does not extend or restart a connection trial already authorized during the window.

`config.restore_defaults` resets operating preferences while preserving saved Wi-Fi, MQTT and calibration configuration. `device.factory_reset` resets configuration, including networks, schedules a restart and preserves administrator authentication, calibration and measurement files. Both require `confirm_device_id`. Forgotten administrator passwords use authorized physical service, not a power-cycle backdoor.

Native fault-injection tests exercise inactive-slot write/commit-pointer interruption and CRC fallback. Physical brownout testing on the manufactured PCB remains outstanding.
