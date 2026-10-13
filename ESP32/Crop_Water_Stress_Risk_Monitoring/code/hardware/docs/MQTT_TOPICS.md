# MQTT topics and access

The approved base is `cws/v1/devices/{device_id}`. The stable identity is `CWS-` followed by the uppercase hexadecimal ESP32 eFuse MAC value. Arbitrary topic prefixes and wildcard characters are rejected.

| Suffix | Device direction | QoS | Retained | Meaning |
|---|---|---:|---|---|
| `telemetry` | Publish | 1 | No | Original measurement records, including offline retry |
| `status` | Publish | 1 | Yes | Latest health/connectivity snapshot |
| `events` | Publish | 1 | No | Boot, setup, connection, calibration, configuration, sensor, power and command events |
| `command_responses` | Publish | 1 | No | Correlated command states/results |
| `configuration` | Publish | 1 | Yes | Sanitized committed configuration on connection |
| `capabilities` | Publish | 1 | Yes | Implemented features and installed optional hardware on connection |
| `commands` | Subscribe | 1 | Must be No | Backend command envelope |
| `sync_ack` | Subscribe | 1 | Must be No | Durable-ingestion acknowledgement |

Inbound retained messages are rejected. Both subscriptions use exact device topics; message envelopes must also target the same device. The broker must deny cross-device publication/subscription and prevent device accounts from publishing commands or acknowledgements.

The backend derives liveness from envelope timestamps and missed health intervals. Retained status alone does not prove that a device is currently online. Broker transport acknowledgement does not prove database ingestion. The durable-ack topic is the only delivery marker accepted by the flash queue when enabled.
