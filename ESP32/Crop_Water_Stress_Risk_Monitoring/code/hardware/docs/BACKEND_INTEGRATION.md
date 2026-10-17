# Backend developer integration

## Registration and authorization

Register the stable device ID, project/site/plot identifiers, hardware revision, firmware version and commissioning/calibration record. Assign a separate broker account to each device. Deliver its hostname, port, username, password and public CA through the local setup process. No customer router or administrator password belongs in the backend registration database.

The backend account may subscribe only to telemetry, events, command responses and the retained dashboard configuration. It may publish only authorized `commands`/`sync_ack` topics. Each device account receives only its own topic ACLs. TLS client certificates are not supported in this build; account credentials plus server-authenticated TLS form the transport identity.

Subscribe to configuration at registration. It contains only the dashboard's editable preferences. Calibration, commissioning, connectivity, diagnostics and optional local peripherals are operated through the authenticated local portal, never inferred or exposed through the dashboard.

## Permanent ingestion

Validate the versioned message, verify topic/envelope device identity, and deduplicate by `message_id`. Commit the complete original record and acquisition metadata durably. Only after that commit, send:

```json
{"schema_version":"1.0","device_id":"CWS-TEST","message_id":"CWS-TEST-0123456789abcdef-1","durably_stored":true}
```

The dashboard configuration update enables durable acknowledgements as part of its storage policy. Do not send ingestion acknowledgements from transient dashboard subscribers. Retries of an already committed identical record receive the same acknowledgement. Conflicting content under an existing ID is an ingestion error.

## Commands

The cloud accepts only `config.patch`. Generate a unique request ID, exact device ID, expiry within 300 seconds, the current revision and the allowlisted dashboard preference fields. Send commands without MQTT retain. Correlate the final response. If the connection drops, repeat the same unexpired request ID; the device suppresses repeated mutation execution. Receipt-only state after power loss is deliberately indeterminate.

History is paginated by exclusive cursor, one record per response. Preserve `next_cursor` even when acknowledgement/purge changes the retained set. A large gap indicates reserved cursor ranges or retention, not fabricated measurements.

## Development tools

Set `CWS_MQTT_HOST`, `CWS_MQTT_PORT`, `CWS_MQTT_CA`, `CWS_MQTT_USERNAME`, `CWS_MQTT_PASSWORD` in the local process environment. Tools enforce CA/hostname verification and never disable TLS.

```powershell
python tools/mqtt_tool.py subscribe --device <device-id>
python tools/mqtt_tool.py responses --device <device-id>
python tools/mqtt_tool.py command --device <device-id> --command config.patch --revision <revision> --args '{"patch":{"upload_s":15}}'
python tools/mqtt_tool.py receive --device <device-id> --directory test-results/backend
python tools/mqtt_tool.py integration --device <device-id> --command config.patch
python tools/validate_schema.py examples/telemetry.json
```

`command` accepts `--args` JSON and `--revision`. `ack --message-id <id>` is an isolated acknowledgement simulator and must not assert permanent ingestion against production records. `receive` writes deduplicated files before sending acknowledgements. `integration` requires a correlated schema-valid `applied` device response over TLS; it fails on timeout/rejection. This live-device test has not been run without hardware/broker credentials.

Acceptance covers disconnect/reconnect, duplicate delivery, wrong-device rejection, failed TLS/ACLs, trial rollback, reboot/power loss, offline backlog, permanently ingested acknowledgements and absent optional peripherals. Host tests are a separate layer, not substitutes for this live acceptance run.
