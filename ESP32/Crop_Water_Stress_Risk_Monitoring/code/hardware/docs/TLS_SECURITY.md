# TLS and credential security

The firmware permits MQTT over TLS only. ESP-MQTT receives the configured CA and broker hostname; common-name/hostname verification is not bypassed. CA replacement parses an X.509 certificate and requires a CA basic constraint before a trial begins. An invalid chain, hostname or connection leaves previous saved settings active. [ESP-MQTT configuration reference](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/protocols/mqtt.html).

The clock bootstraps using SNTP from `pool.ntp.org` and `time.cloudflare.com`. TLS attempts wait for a plausible Unix time. Unsynchronized measurements retain boot-relative time with `unix_s: null`. SNTP is not authenticated; deployments must provide trustworthy time/network infrastructure. Remote commands are rejected while the clock is unsynchronized and expire within at most 300 seconds of receipt.

Broker ACLs grant each device publish access to its own telemetry/status/events/response/configuration/capability topics and subscribe access to its own commands/sync_ack topics. Only authorized backend identities may publish commands or durable acknowledgements. This is the command authorization boundary; messages do not carry a separate application signature.

The local portal is HTTP inside a password-protected, temporary WPA2 AP. Host, origin, interface, session and CSRF checks reduce local web attacks. Possession of the AP password still grants network adjacency; it is a private credential.

Administrator replacements are salted verifiers. Router/MQTT passwords and the compiled setup password must remain recoverable by the firmware and are not encrypted by this standard Arduino build. Flash encryption, secure boot, hardware tamper protection, mutual TLS and OTA are not enabled. Production physical-access controls remain an explicit commissioning requirement.

Credentials, private keys and tokens are excluded from telemetry, configuration exports and diagnostic messages. Public CA certificates can be tracked; private-key and provisioning paths are explicitly ignored without a blanket `*.pem` exclusion.
