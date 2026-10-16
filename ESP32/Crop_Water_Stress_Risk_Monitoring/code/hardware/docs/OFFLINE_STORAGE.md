# Offline storage and synchronization

LittleFS contains pending `/records/<cursor>.rec` files. Each record has a magic value, byte length, CRC32, monotonic history cursor and the original JSON payload. A temporary file is flushed and renamed before it enters the in-memory index. Startup scans and verifies finalized files, discards incomplete temporary writes and preserves corrupt records for service rather than automatically formatting the filesystem.

History cursors are reserved in NVS in batches of 64 before use. Gaps are valid; a reboot cannot reuse an earlier cursor. Record identity is independent of this cursor and remains stable through retransmission. Startup verifies the remaining pending files and recovers the next cursor safely.

Retention is 8-256 records, default 256, shared by telemetry and events. A durable backend acknowledgement removes its record immediately. If the device remains offline and the queue fills, `drop_oldest: true` discards the oldest pending record so fresh readings can still be queued. Capacity also reserves 16 KiB free space for filesystem operation. Overflow is reported, never hidden as successful storage.

Synchronization starts on MQTT connection, at the configured upload interval, or by an explicit retry command. At most one queued record is offered per second. The original topic category, ID, timestamp and payload are preserved. A disconnected/full MQTT queue leaves the cursor eligible for retry. A synchronization pass is restarted periodically; without backend acknowledgements this intentionally republishes retained history.

`storage.durable_ack` defaults true. The device accepts only an acknowledgement for the exact target device, schema `1.0`, a known record ID and `durably_stored: true`. It then deletes that record from LittleFS, allowing the stored-record count to return to zero after successful synchronization. Unknown records do nothing. A failed delete keeps the record pending for retry. MQTT PUBACK alone never deletes or marks a record delivered.

`telemetry.get_history` and `storage.export_records` accept `after` (exclusive cursor) and optional `limit: 1`. Results contain `records`, `next_cursor` and `more`. Each row includes `index`, `acknowledged` and the original `record`. With durable acknowledgement enabled, delivered records are no longer present in local history; the backend is their permanent record.

Counters labelled `since_boot` are volatile. The file-based receiver performs a flush/fsync and atomic replace before acknowledgement and rejects conflicting payloads under the same identity. Filesystem/controller power-loss guarantees still depend on the host platform. Native adapter tests exercise interruption and recovery logic; physical brownout endurance remains unverified.
