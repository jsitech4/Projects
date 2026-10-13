# Version 1.0 JSON contract

Checked-in JSON Schemas use draft 2020-12. `tools/validate_schema.py` validates individual messages. `tools/export_contract.py` maintains the command registry, schema files and isolated fixtures. The fixture device `CWS-TEST` is not a deployed unit.

All device-originated messages include `schema_version`, `message_type`, `message_id`, `device_id`, `boot_id`, `sequence` and `time`. A message ID combines device identity, a random 64-bit boot ID and a per-boot sequence. Sequences span message categories; telemetry sequences are not consecutive by themselves.

`time.uptime_ms` is monotonic 64-bit elapsed time. `time.synchronized` indicates plausible UTC; `time.unix_s` is null until synchronization. The stored envelope time is the record acquisition/completion time. Sensor objects retain their individual acquisition uptime, and environment age describes its last valid frame. Historical records retain the original envelope unchanged, including an unsynchronized null UTC timestamp.

| Schema | Payload |
|---|---|
| `telemetry.schema.json` | Environment, soil, battery, solar, identity, revisions, connectivity, queue, health and peripherals |
| `status.schema.json` | Current identity/connectivity/storage/health without a new measurement |
| `event.schema.json` | Structured event code and envelope |
| `configuration.schema.json` | Sanitized committed settings |
| `capabilities.schema.json` | Implemented commands, detected optional hardware and protocol limits |
| `command.schema.json` | Request ID, target, command-specific arguments, expiration and expected revision |
| `command_response.schema.json` | Request correlation, execution status, result, error and resulting revision |
| `sync_ack.schema.json` | Exact record identity and permanent-ingestion assertion |
| `configuration_patch.schema.json` | Allowlisted configuration fields and bounds |

Engineering values that cannot be defended are null. Raw ADC fields remain integers even when the conversion is invalid; the paired validity and quality flags are authoritative. `vwc_pct` and battery `percentage` are always null in this firmware. ADC millivolts report whether factory eFuse calibration was available.

Connectivity includes Wi-Fi/IP/RSSI, MQTT subscription state, TLS verification, reconnect count, transport error and broker publication acknowledgement. Queue statistics include retained/pending counts, bytes, oldest pending identity, last application acknowledgement, retry count and drops/faults since boot. Health includes reset reason, boot count, heap, physical flash size and active faults.

Payloads must fit 8,192 bytes. History responses contain one stored record and a cursor. Consumers must tolerate asynchronous responses and QoS 1 duplicates, using `message_id` for ingestion deduplication and `request_id` for command correlation.
