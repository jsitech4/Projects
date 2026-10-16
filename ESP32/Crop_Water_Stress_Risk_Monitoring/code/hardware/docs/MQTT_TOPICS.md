# MQTT topics and access

The approved base is `cws/v1/devices/{device_id}`. The stable identity is `CWS-` followed by the uppercase hexadecimal ESP32 eFuse MAC value. Arbitrary topic prefixes and wildcard characters are rejected.

| Suffix | Device direction | QoS | Retained | Meaning |
|---|---|---:|---|---|
| `telemetry` | Publish | 1 | No | Dashboard measurements: soil moisture, air temperature/humidity, battery and solar voltage, sensor validity, timing and device identifiers; includes offline retry |
| `events` | Publish | 1 | No | Boot, setup, connection, calibration, configuration, sensor, power and command events |
| `command_responses` | Publish | 1 | No | Correlated command states/results |
| `configuration` | Publish | 1 | Yes | Dashboard preferences only: revision, telemetry interval, LED, buzzer and logging retention |
| `commands` | Subscribe | 1 | Must be No | Backend command envelope |
| `sync_ack` | Subscribe | 1 | Must be No | Durable-ingestion acknowledgement |

Inbound retained messages are rejected. Both subscriptions use exact device topics; message envelopes must also target the same device. The broker must deny cross-device publication/subscription and prevent device accounts from publishing commands or acknowledgements.

The backend derives liveness from recent telemetry timestamps. Broker transport acknowledgement does not prove database ingestion. The durable-ack topic is the only delivery marker accepted by the flash queue when enabled.

The only accepted cloud command is revision-checked `config.patch`; it may change the dashboard's telemetry interval, LED, buzzer and storage policy. Network credentials, calibration, diagnostics, raw ADC values, Wi-Fi identity, IP address, broker host, storage counters and local peripheral state never cross the dashboard contract.
