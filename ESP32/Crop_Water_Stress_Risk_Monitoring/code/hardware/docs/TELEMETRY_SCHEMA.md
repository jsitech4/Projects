# Version 1.0 JSON contract

Checked-in JSON Schemas use draft 2020-12. `tools/validate_schema.py` validates individual messages. `tools/export_contract.py` maintains the command registry, schema files and isolated fixtures. The fixture device `CWS-TEST` is not a deployed unit.

All device-originated messages include `schema_version`, `message_type`, `message_id`, `device_id`, `boot_id`, `sequence` and `time`. A message ID combines device identity, a random 64-bit boot ID and a per-boot sequence. Sequences span message categories; telemetry sequences are not consecutive by themselves.

`time.uptime_ms` is monotonic 64-bit elapsed time. `time.synchronized` indicates plausible UTC; `time.unix_s` is null until synchronization. The stored envelope time is the record acquisition/completion time. Historical records retain the original envelope unchanged, including an unsynchronized null UTC timestamp.

| Schema | Payload |
|---|---|
| `telemetry.schema.json` | Dashboard values only: environment, soil, battery, solar, identity, revision and timing |
| `status.schema.json` | Local/diagnostic schema; not published to the dashboard contract |
| `event.schema.json` | Structured event code and envelope |
| `configuration.schema.json` | Dashboard preferences only: revision, upload interval, LED, buzzer and record retention |
| `capabilities.schema.json` | Local/protocol reference; not published to the dashboard contract |
| `command.schema.json` | Request ID, target, command-specific arguments, expiration and expected revision |
| `command_response.schema.json` | Request correlation, execution status, result, error and resulting revision |
| `sync_ack.schema.json` | Exact record identity and permanent-ingestion assertion |
| `configuration_patch.schema.json` | Allowlisted configuration fields and bounds |

Engineering values that cannot be defended are null. The dashboard receives calibrated display values and a validity flag for each installed sensor. Raw ADC, Wi-Fi/IP/RSSI, broker details, queue statistics, health diagnostics, local peripheral state and calibration internals are intentionally excluded from cloud telemetry.

Payloads must fit 8,192 bytes. History responses contain one stored record and a cursor. Consumers must tolerate asynchronous responses and QoS 1 duplicates, using `message_id` for ingestion deduplication and `request_id` for command correlation.
