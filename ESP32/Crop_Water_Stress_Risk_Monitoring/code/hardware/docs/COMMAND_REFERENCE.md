# Remote command reference

All requests use schema `1.0`, an exact device ID, a unique 1–64 character alphanumeric/underscore/hyphen request ID, an argument object and a Unix expiry no more than 300 seconds ahead. The broker authenticates and authorizes the sender; retained commands are rejected.

Commands marked **Yes** require `expected_revision`. Destructive commands also require `confirm_device_id` equal to the target. Unknown arguments are rejected. Optional arguments are marked `?`; `{}` means no arguments. The checked-in command schema contains the types and numerical bounds.

Read operations return snapshots or validation results. Configuration and preference setters return the committed revision. Calibration is restricted to the local developer portal. Acquisition and connection trials return `accepted` before a final response. Bounded LED/buzzer tests report that the operation started; they do not establish electrical functionality.

Mutation receipts are written before execution, with final status/error persisted afterward. Repeating an ID returns the stored status without executing again. A `received` receipt after reset has `execution_indeterminate_after_restart`; query the actual state before using a new ID. Accepted operations can be interrupted by reset. Read-only requests can be safely re-executed. There are 32 active mutation receipts, each retained through expiry, and a one-second mutation rate limit. Full replay windows reject new mutations.

## Device

| Command | Revision required | Arguments |
|---|---|---|
| `device.get_info` | No | `{}` |
| `device.get_status` | No | `{}` |
| `device.get_health` | No | `{}` |
| `device.get_capabilities` | No | `{}` |
| `device.reboot` | Yes | `confirm_device_id` |
| `device.factory_reset` | Yes | `confirm_device_id` |
| `device.clear_recoverable_faults` | Yes | `{}` |
| `device.get_diagnostics` | No | `{}` |
| `device.sync_time` | Yes | `{}` |

## Config

| Command | Revision required | Arguments |
|---|---|---|
| `config.get` | No | `{}` |
| `config.validate` | No | `patch` |
| `config.patch` | Yes | `patch` |
| `config.restore_defaults` | Yes | `confirm_device_id` |
| `config.get_revision` | No | `{}` |
| `config.export` | No | `{}` |

## Environment

| Command | Revision required | Arguments |
|---|---|---|
| `environment.read_now` | Yes | `{}` |
| `environment.get_status` | No | `{}` |

## Soil

| Command | Revision required | Arguments |
|---|---|---|
| `soil.read_now` | Yes | `{}` |
| `soil.get_status` | No | `{}` |

## Battery

| Command | Revision required | Arguments |
|---|---|---|
| `battery.read_now` | Yes | `{}` |
| `battery.get_status` | No | `{}` |

## Solar

| Command | Revision required | Arguments |
|---|---|---|
| `solar.read_now` | Yes | `{}` |
| `solar.get_status` | No | `{}` |

## Lcd

| Command | Revision required | Arguments |
|---|---|---|
| `lcd.get_status` | No | `{}` |
| `lcd.set_backlight` | Yes | `backlight` |
| `lcd.set_timeout` | Yes | `timeout_s` |
| `lcd.set_page` | Yes | `page` |
| `lcd.set_enabled` | Yes | `enabled` |
| `lcd.show_notice` | Yes | `text`, `duration_ms` |

## Led

| Command | Revision required | Arguments |
|---|---|---|
| `led.get_status` | No | `{}` |
| `led.set_enabled` | Yes | `enabled` |
| `led.set_preferences` | Yes | `enabled?`, `active_high?`, `breathing?` |
| `led.test` | Yes | `duration_ms` |
| `led.reset_preferences` | Yes | `{}` |

## Buzzer

| Command | Revision required | Arguments |
|---|---|---|
| `buzzer.get_status` | No | `{}` |
| `buzzer.set_enabled` | Yes | `enabled` |
| `buzzer.set_alert_preferences` | Yes | `enabled?`, `passive?`, `active_high?`, `alerts?` |
| `buzzer.mute` | Yes | `duration_s` |
| `buzzer.unmute` | Yes | `{}` |
| `buzzer.test` | Yes | `duration_ms` |
| `buzzer.stop` | Yes | `{}` |

## Wifi

| Command | Revision required | Arguments |
|---|---|---|
| `wifi.get_status` | No | `{}` |
| `wifi.scan` | Yes | `{}` |
| `wifi.configure` | Yes | `settings` |
| `wifi.test_connection` | Yes | `settings` |
| `wifi.reconnect` | Yes | `{}` |
| `wifi.disconnect` | Yes | `{}` |
| `wifi.forget_network` | Yes | `confirm_device_id` |

## Mqtt

| Command | Revision required | Arguments |
|---|---|---|
| `mqtt.get_status` | No | `{}` |
| `mqtt.configure` | Yes | `settings` |
| `mqtt.test_connection` | Yes | `settings` |
| `mqtt.reconnect` | Yes | `{}` |
| `mqtt.get_config` | No | `{}` |

## Telemetry

| Command | Revision required | Arguments |
|---|---|---|
| `telemetry.read_now` | Yes | `{}` |
| `telemetry.get_last` | No | `{}` |
| `telemetry.get_history` | No | `after`, `limit?` |
| `telemetry.publish_now` | Yes | `{}` |
| `telemetry.set_intervals` | Yes | `sample_s?`, `upload_s?`, `health_s?` |
| `telemetry.get_delivery_status` | No | `{}` |
| `telemetry.retry_pending` | Yes | `{}` |

## Storage

| Command | Revision required | Arguments |
|---|---|---|
| `storage.get_status` | No | `{}` |
| `storage.get_statistics` | No | `{}` |
| `storage.get_pending_count` | No | `{}` |
| `storage.export_records` | No | `after`, `limit?` |
| `storage.retry_sync` | Yes | `{}` |
| `storage.set_retention` | Yes | `retain?`, `drop_oldest?`, `durable_ack?` |
| `storage.purge_acknowledged` | Yes | `confirm_device_id` |

## Diagnostics

| Command | Revision required | Arguments |
|---|---|---|
| `diagnostics.get_faults` | No | `{}` |
| `diagnostics.get_logs` | No | `{}` |
| `diagnostics.self_test` | No | `{}` |
| `diagnostics.get_memory` | No | `{}` |
| `diagnostics.get_reset_reason` | No | `{}` |

## Firmware

| Command | Revision required | Arguments |
|---|---|---|
| `firmware.get_version` | No | `{}` |
| `firmware.get_update_status` | No | `{}` |

## Execution details

- `read_now` acquires the complete sensor set and completes only after a record-storage attempt. Get the resulting data with `telemetry.get_last` or the corresponding sensor status command.
- Sensor status commands expose observed readings and quality only. Calibration is local-developer-portal-only; remote configuration exports do not contain calibration settings. See [Calibration](CALIBRATION.md).
- `wifi.scan` returns the previous scan results and requests a new asynchronous scan. A subsequent call retrieves the completed results.
- `wifi.configure` / `mqtt.configure` save only after their selected trial passes. `*.test_connection` restores previous settings after the trial.
- `device.sync_time` requests SNTP and reports the current synchronization state; it does not claim a completed time adjustment.
- `lcd.set_enabled` configures the optional profile even when absent. Other LCD mutations require detected hardware. Encoder enable is available through `config.patch`.
- LED/buzzer tests require 20–3,000 ms. Buzzer tests reject disabled or muted output. Notices require 1,000–60,000 ms and at most 80 characters.
- `storage.export_records` / `telemetry.get_history` use exclusive `after` and `limit: 1`; `more` and `next_cursor` describe pagination.
- Factory reset preserves records, the administrator verifier and calibration. Restore-defaults also preserves Wi-Fi/MQTT and calibration. See [Recovery](CONFIGURATION_RECOVERY.md).
- Firmware check/start-update commands are unsupported and not advertised; updates use physical serial upload. No command executes arbitrary code, file paths or GPIO access.

## Response and error contract

Every response carries the originating request ID, `status`, `error`, `result`, resulting configuration revision and normal message envelope. Status values are `received`, `accepted`, `applied`, `rejected`, `failed` and reserved `timed_out`; connection timeouts currently produce `failed` with `connection_trial_failed`. Responses lost during a connection change can be recovered by repeating the same unexpired ID after reconnect.

Envelope errors: `invalid_json`, `wrong_version_or_device`, `invalid_envelope`, `clock_unsynchronized`, `expired_or_future`. Authorization/dispatch errors: `unsupported_command`, `revision_conflict`, `confirmation_required`, `unknown_argument`, `command_rate_limited`, `replay_window_full`, `replay_storage_unavailable`, `replay_journal_write_failed`. Operational errors include `configuration_trial_busy`, `connection_trial_failed`, `trial_cancelled`, `configuration_commit_failed`, `unsupported_hardware`, `acquisition_busy`, `sensor_minimum_interval`, `fresh_valid_adc_required`, `record_not_stored` and `response_too_large`. Validation errors identify the field group; successful responses carry an empty error string.
