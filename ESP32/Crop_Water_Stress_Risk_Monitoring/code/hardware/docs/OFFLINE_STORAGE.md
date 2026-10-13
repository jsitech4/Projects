# Offline storage and synchronization

LittleFS contains `/records/<cursor>.rec` and acknowledged `.ack` files. Each record has a magic value, byte length, CRC32, monotonic history cursor and the original JSON payload. A temporary file is flushed and renamed before it enters the in-memory index. Startup scans and verifies finalized files, discards incomplete temporary writes and preserves corrupt records for service rather than automatically formatting the filesystem.

History cursors are reserved in NVS in batches of 64 before use. Gaps are valid; a reboot or purge cannot reuse an earlier cursor. Record identity is independent of this cursor and remains stable through retransmission. Startup restores acknowledgement state from filenames and recovers the latest acknowledged identity.

Retention is 8–256 records, default 256, shared by telemetry and events. With `drop_oldest: false`, acknowledged records are evicted first; a full queue of pending records rejects new records and increments its drop counter. With explicit `drop_oldest: true`, the oldest pending record can be discarded. Capacity also reserves 16 KiB free space for filesystem operation. Overflow is reported, never hidden as successful storage.

Synchronization starts on MQTT connection, at the configured upload interval, or by an explicit retry command. At most one queued record is offered per second. The original topic category, ID, timestamp and payload are preserved. A disconnected/full MQTT queue leaves the cursor eligible for retry. A synchronization pass is restarted periodically; without backend acknowledgements this intentionally republishes retained history.

`storage.durable_ack` defaults false. When enabled, the device accepts only the exact target device, schema `1.0`, known record ID and `durably_stored: true`. Renaming the record to `.ack` is the persistent delivery marker. Unknown records do nothing. A failed rename keeps the record pending. MQTT PUBACK alone never deletes or marks a record delivered.

`telemetry.get_history` and `storage.export_records` accept `after` (exclusive cursor) and optional `limit: 1`. Results contain `records`, `next_cursor` and `more`. Each row includes `index`, `acknowledged` and the original `record`. Purging acknowledged records requires explicit device-ID confirmation. Unacknowledged records are never purged by that command.

Counters labelled `since_boot` are volatile. The file-based receiver performs a flush/fsync and atomic replace before acknowledgement and rejects conflicting payloads under the same identity. Filesystem/controller power-loss guarantees still depend on the host platform. Native adapter tests exercise interruption and recovery logic; physical brownout endurance remains unverified.
