# Architecture

```text
ESP32 device --MQTTS QoS 1--> MQTT broker --MQTTS QoS 1--> ingestion worker
                                                        | schema/topic validation
                                                        v
                                                PostgreSQL transaction
                                                        |
                              only after successful commit: sync_ack --> broker --> ESP32

Browser --> HTTPS reverse proxy --> Next.js dashboard --> HTTPS API --> PostgreSQL
                                                        \--> MQTT command publisher --> broker --> ESP32
```

The backend is intentionally the permanent-ingestion authority. MQTT transport acknowledgement is not storage acknowledgement. A duplicate `message_id` is safe: it has already been durably committed and receives the same `sync_ack` again.

The worker receives only the six device-originated topic suffixes. It validates the exact topic device ID against the envelope, validates the firmware's JSON schema, records the original payload, normalizes telemetry, calculates a transparent risk assessment, then publishes the acknowledgement after the database transaction resolves.

Risk assessment thresholds are supplied by the crop profile assigned to the device plot. A missing profile, stale record, failed calibration, or invalid sensor frame produces `UNKNOWN`; the system does not apply a global crop threshold to the firmware's relative-moisture index.

`capabilities` controls future command UI expansion. Optional LCD and encoder controls must not be inferred from dashboard settings.
