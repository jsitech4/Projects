# System architecture

`App` owns all application services. The Arduino loop schedules acquisition, bounded HTTP work, trial completion, incoming commands, offline synchronization, indicators and optional display refresh. Configuration and filesystem mutations occur on this task. MQTT callbacks transfer bounded messages through FreeRTOS queues; they never edit calibration or files.

`Network` owns an independent worker and the framework ESP-MQTT task. DNS, TLS handshakes, broker retry and transport teardown do not block acquisition. Its four-message publish queue and four-message receive queue reject excess work. Payloads are limited to 8,192 bytes and the MQTT outbox is bounded before enqueueing. Configuration requests cross a separate queue with generation tracking, so a previous connection cannot validate a new broker trial.

`Security` performs PBKDF2 in an idle-priority worker. The application task commits authentication results only when the originating job is polled during the setup window. Abandoned jobs expire; portal shutdown cancels them. AM2301A start pulses temporarily defer HTTP handling, and response capture bounds each pulse wait to 110 microseconds.

The source dependency flow is Board/Core → individual services → System. Command dispatch resides with the System coordinator and uses the same validators and commit path as local configuration. No peripheral is a prerequisite for boot.

Measurement/event envelopes receive random boot IDs and monotonically increasing per-boot sequence numbers. Stored envelopes retain their original identities and timestamps when retried. Diagnostics and capability messages distinguish configured features, detected peripherals, missing calibration and runtime failure.

The filesystem contains records only. HTML/CSS/JavaScript are compiled into the application. Configuration, credential verifiers, replay receipts and cursor reservations occupy separate NVS namespaces. [Persistence](CONFIGURATION_PERSISTENCE.md) describes their commit boundaries.
