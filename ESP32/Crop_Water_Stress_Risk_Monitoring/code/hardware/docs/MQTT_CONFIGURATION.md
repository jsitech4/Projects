# MQTT connection configuration

`mqtt` contains a DNS hostname (maximum 128 characters), port (1–65,535; default 8,883), device username (64 characters), password (128 characters), PEM public CA (6,000 bytes), fixed prefix `cws/v1/devices` and timeout (5–30 seconds). No registrar, database or server-root credentials are used.

ESP-MQTT publishes and subscribes with QoS 1. It restores command and acknowledgement subscriptions after reconnection. Retained inbound commands/acknowledgements and messages over 8,192 bytes are rejected. Subscription acknowledgement and MQTT PUBACK are transport observations, not permanent backend-ingestion proof.

Broker trials allow 60 seconds for DNS, time availability, TCP/TLS, authentication and subscription. A previous MQTT connection cannot validate a newly queued configuration. Test-only operation restores the saved broker. Save commits only a successful trial. Wrong credentials or certificates never enable insecure fallback.

Connection error values are: 0 none, 1 broker/CA absent, 2 clock unavailable, 3 DNS failure, 4 TCP/socket failure, 5 TLS failure, 6 broker refusal/authentication failure, 7 other MQTT error, 8 client allocation failure. Retries back off to five minutes. Networking runs outside the acquisition task.

The device client ID is its stable `CWS-<MAC>` identity. Topic authorization belongs to the broker ACL. [Topics](MQTT_TOPICS.md) and [TLS security](TLS_SECURITY.md) define the external requirements. Direct ESP32/broker testing is an outstanding hardware acceptance item.
