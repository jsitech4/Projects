# Firmware verification report

Verification date: **13 October 2026** (Africa/Lagos).

Project root: `C:\Users\JSI Tech\Documents\My Kicad File\Github\KiCad_Design\Projects\ESP32\Crop_Water_Stress_Risk_Monitoring\code\hardware`.

The existing modular firmware was retained and reviewed in place. The application builds for the original ESP32, and the available host suites pass. Physical commissioning and live ESP32/broker acceptance remain outstanding. This report does not establish field readiness.

## Changes verified

- Configuration fallback repairs the persisted commit pointer before reusing a damaged slot. Failed pointer repair blocks candidate writes. Recovery to defaults writes an explicit revision-zero pointer. Regression cases cover interrupted saves following both recovery paths.
- Expired buzzer and LED test operations clear their timers, preventing reactivation when `millis()` wraps. Buzzer enable, polarity and active/passive changes apply even while idle. Disabling the buzzer cancels a pending beep.
- Private commissioning requires a punctuation character, matching the firmware password policy. Whitespace alone does not satisfy that requirement.
- Repository and firmware `.gitignore` rules preserve existing exclusions and add static libraries, dependency files, coverage intermediates and Python analysis caches. README and persistence/testing documentation describe the final behavior.

## Build and test evidence

| Check | Result | Scope |
|---|---|---|
| `pio run -e esp32_wroom_32u` | Passed; final build 26.68 s | Dependency resolution, compilation, linking, image generation and application size |
| `pio test -e native` | 27 passed | Pin lock, protocol decoding, calculations, validation, redaction, deadlines and command envelopes |
| `python tools/test_persistence.py` | 13 persistence tests passed | Production configuration/storage logic with injected NVS/filesystem failures |
| Same host runner | 10 HTTP/lifecycle tests passed | Production HTTP parser, response writes, interface restrictions and setup expiry |
| Same host runner | 5 indicator tests passed | Production indicator logic with GPIO/PWM adapters |
| `CWS_TEST_BROWSER=chrome` and pytest integration suite | 35 passed, 15.49 s | 24 protocol/tooling cases and 11 browser cases |
| `node --check data/portal/app.js` | Passed | JavaScript syntax |
| `git diff --check` | Passed | Patch whitespace |

The first browser run encountered an Edge launch failure before any browser case executed. Selecting the installed Chrome channel resolved the runner failure; no browser tests were skipped. Layout assertions cover 320, 768 and 1280 px widths. The mobile Wi-Fi and desktop MQTT screenshots were also visually inspected. Screenshots and JUnit output reside in ignored `test-results/`.

Browser authentication and connection responses are fixtures. These checks establish UI behavior, not execution of ESP32 password hashing, radio association, NVS drivers or TLS. The host certificate adapter deliberately rejects certificates. PBKDF2 provisioning output is checked with Python's cryptographic implementation; on-device authentication and session acceptance still require physical verification.

## Resource measurements

| Resource | Measured or configured value |
|---|---:|
| Linked application flash | 1,102,057 / 2,097,152 bytes, 52.6% |
| Static RAM | 77,152 / 327,680 bytes, 23.5% |
| Generated `firmware.bin` | 1,108,640 bytes |
| HTML asset | 8,343 bytes |
| CSS asset | 5,136 bytes |
| JavaScript asset | 10,878 bytes |
| Total source portal assets | 24,357 bytes |
| Configured physical flash | 4,194,304 bytes; hardware confirmation pending |
| NVS partition | 86,016 bytes |
| Application partition | 2,097,152 bytes |
| LittleFS partition | 1,966,080 bytes |

Firmware SHA-256: `66236d64420a0dd5c7b26a5fd2efc9833a99b99fe9b93a66c7f71c565f65b1aa`.

The verified image contains **no factory commissioning material**. It intentionally leaves the setup AP closed until built with private per-device material. Commissioned images have different hashes and resource sizes and require a fresh build/size check.

PlatformIO's 2 MB flash banner reflects the configured application maximum; the image header and partition table target 4 MB physical flash. Static RAM accounting does not measure peak TLS heap, task stacks or fragmentation. Runtime health messages provide free/minimum heap. The single application partition does not support OTA.

## Implemented interfaces and modules

| Area | Implementation and contract |
|---|---|
| Board | `lib/Board/Pins.h` and `.cpp`; compile-time pin assertions and native checks. GPIO32/34/35/39 sensors, 21/22 LCD, 25/26/27 encoder, 2 LED, 4 buzzer. Excluded pump/tank GPIOs are not controlled. |
| Sensors and calibration | AM2301A bounded frame capture/checksum, filtered ADC1 acquisition, both soil orientations, independent battery/solar divider profiles and stored offsets. Invalid engineering readings are null. See [acquisition](SENSOR_ACQUISITION.md) and [calibration](CALIBRATION.md). |
| Headless/optional hardware | Default headless profile; explicit LCD/encoder settings use the same application. LCD address probing, three status pages and encoder navigation are conditional. See [headless operation](HEADLESS_OPERATION.md). |
| Local indicators | Non-blocking LED patterns and bounded buzzer output, enable/mute and fault priority. Buzzer remains disabled by default pending electrical commissioning. |
| Provisioning and portal | Unique WPA2 setup credential; commissioned units open a 300-second monotonic window on every restart. Server/AP closure preserves normal operation. Login, overview, Wi-Fi, MQTT/TLS, security and diagnostics forms are included. See [portal](LOCAL_CONFIGURATION_PORTAL.md). |
| Administrator credentials | Salted PBKDF2 verifier, required initial replacement, current-password verification, session invalidation, CSRF, login throttling and secured physical-service recovery. See [authentication](ADMINISTRATOR_AUTHENTICATION.md). |
| Configuration | NVS snapshots, CRC, revision checks, connection trials, rollback and recovery. Wi-Fi can be saved with backend validation pending. See [persistence](CONFIGURATION_PERSISTENCE.md) and [recovery](CONFIGURATION_RECOVERY.md). |
| Wi-Fi and MQTT | AP+STA provisioning, DHCP/static IPv4, scanning, retries, ESP-MQTT QoS 1, CA/hostname verification, time bootstrap, device identity and bounded message queues. See [Wi-Fi](WIFI_CONFIGURATION.md), [MQTT](MQTT_CONFIGURATION.md) and [TLS](TLS_SECURITY.md). |
| Telemetry | Version 1.0 environmental, soil, battery, solar, identity, connectivity, storage, health and peripheral metadata; status, events, sanitized configuration and capabilities. See [schemas](TELEMETRY_SCHEMA.md). |
| Commands | 74 implemented remote command contracts across device/configuration, sensor status, LCD, indicators, connectivity, telemetry, storage, diagnostics and firmware information. Calibration is restricted to the local developer portal. Target, expiry, mutation revision, replay receipts and destructive confirmation checks apply. Absent hardware is reported explicitly. See [command reference](COMMAND_REFERENCE.md). |
| Offline storage | Bounded LittleFS queue, CRC, stable record IDs, interruption recovery, paginated history, retention and retry. MQTT PUBACK alone never authorizes durable deletion. See [storage](OFFLINE_STORAGE.md). |
| Backend handover | JSON schemas, sanitized examples, topics, capabilities, registration/ACL rules, durable-ingestion acknowledgement and deduplication contract. Tooling supplies subscriber, publisher, response inspector, acknowledgement simulator, durable file receiver and live integration runner. See [backend integration](BACKEND_INTEGRATION.md). |

OTA update commands, mutual TLS, scientific VWC and battery state-of-charge percentage are not advertised as implemented capabilities. No external monitoring website is included.

## File and credential hygiene

A scan of 94 tracked firmware-project files found no tracked private provisioning paths, private-key markers or credential-shaped literal assignments outside isolated fixtures. Generated factory headers, embedded assets and firmware binaries are ignored. The scan covers the current project working tree; it is not a repository-history secret audit. No credential values were printed. Existing untracked IDE files remain intact.

## Physical commissioning and acceptance still required

1. Confirm physical flash capacity, PCB revision, supply integrity and external antenna installation.
2. Measure the actual battery/solar resistor dividers and linear ADC span, including Zener clipping. Confirm sensor logic levels, pull-up and exact AM2301A timing. The conservative ADC span follows [Espressif's original ESP32 ADC reference](https://docs.espressif.com/projects/esp-idf/en/v4.4.8/esp32/api-reference/peripherals/adc.html); it does not establish the PCB transfer function.
3. Verify LED/buzzer polarity and driver current, boot behavior and optional LCD backpack mapping/I²C voltage. Confirm headless boot and missing-peripheral behavior on both hardware populations.
4. Commission unique credentials, initialize blank storage and verify the 300-second AP/server shutdown with saved and unsaved connection settings while measurements continue.
5. Verify password replacement across reboot, rejection of the old password, throttling, CSRF/session expiry and authorized serial recovery on the ESP32.
6. Exercise valid and invalid router settings, shared-radio channel changes, static-IP conflicts, failed MQTT trials, CA/hostname rejection and broker ACL denial. Measure peak heap during overlapping TLS, portal, authentication and storage activity.
7. Exercise live commands, durable acknowledgements, disconnect/reconnect synchronization, queue overflow and repeated power interruption during real flash writes. Validate backend deduplication against actual device messages.

No physical upload, electrical measurement, sensor timing capture, live broker acceptance or field deployment occurred during this verification. Secure boot and flash encryption are not enabled by the standard build.

## Commissioning and upload commands

From the project root, create each device's private record once:

```powershell
.\.venv\Scripts\python.exe tools/commission.py create --directory provisioning/private/unit-001
$env:CWS_FACTORY_FILE = (Resolve-Path provisioning/private/unit-001/factory.json).Path
pio run -e esp32_wroom_32u
pio device list
pio run -e esp32_wroom_32u -t upload
pio device monitor -b 115200
```

For an existing unit, select its existing `factory.json` instead of creating new credentials. PlatformIO selects the serial port; an explicit `--upload-port <identified-port>` can be supplied when needed. Factory storage initialization and owner/service record handling are defined in [factory commissioning](FACTORY_COMMISSIONING.md). Portal assets are part of the application; `uploadfs` is not part of routine deployment.

The [README](../README.md), [system architecture](SYSTEM_ARCHITECTURE.md), [hardware](HARDWARE.md), [pin mapping](PIN_MAPPING.md), [testing procedure](TESTING.md) and [deployment procedure](DEPLOYMENT.md) complete the operational handover.
