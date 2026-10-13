# Crop Water-Stress Risk Monitoring in 5G Networks

Headless ESP32-WROOM-32U firmware for environmental, relative soil moisture, battery and solar voltage monitoring. A 2.4 GHz Wi-Fi router supplies internet access; the router, rather than the ESP32, handles any 4G/5G cellular link. The external monitoring website and database are separate systems.

The firmware provides a temporary local connectivity portal, MQTT over verified TLS, versioned JSON commands, persistent calibration, and a bounded offline record queue. One application binary supports headless hardware and explicitly enabled LCD2004/encoder hardware.

## Build and verification

The project uses PlatformIO Core, Espressif32 **6.12.0**, Arduino-ESP32 **2.0.17**, ArduinoJson **6.21.5**, and the framework's ESP-MQTT client. The `esp32dev` board definition targets the original ESP32. The partition table targets **4 MiB physical flash**, with a **2 MiB application** and **1.875 MiB LittleFS**. Physical capacity still requires commissioning verification. OTA is not supported by this partition layout.

```powershell
pio run -e esp32_wroom_32u
pio test -e native
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r tools/requirements.txt
.\.venv\Scripts\python.exe tools/test_persistence.py
.\.venv\Scripts\python.exe -m pytest test/integration -q
```

Native tests require a C++17 `g++` on PATH. Browser tests use Microsoft Edge by default; `CWS_TEST_BROWSER=chrome` selects installed Chrome. The host adapters exercise production storage/configuration/HTTP code; they do not emulate the ESP32's electrical behavior. Results and outstanding checks are recorded in [Testing](docs/TESTING.md).

## Fixed PCB connections

| Component | GPIO |
|---|---:|
| AM2301A data | 32 |
| Soil ADC1 | 34 |
| Battery ADC1 | 35 |
| Solar ADC1 | 39 |
| LCD SDA / SCL | 21 / 22 |
| Encoder CLK / DT / switch | 25 / 26 / 27 |
| Status LED | 2 |
| Buzzer | 4 |

GPIO14, GPIO12, GPIO33 and GPIO16 remain outside this application's control. `lib/Board/Pins.h` contains compile-time checks. No pump, tank, motor or arbitrary GPIO command exists. See [Hardware](docs/HARDWARE.md) and [Pin mapping](docs/PIN_MAPPING.md).

## Factory commissioning and upload

Uncommissioned builds compile and acquire measurements but **do not open an access point**. There is no shared default password. Private per-device input supplies a unique WPA2 setup password, a salted initial administrator verifier and a separate service-token verifier.

```powershell
.\.venv\Scripts\python.exe tools/commission.py create --directory provisioning/private/unit-001
$env:CWS_FACTORY_FILE = (Resolve-Path provisioning/private/unit-001/factory.json).Path
pio run -e esp32_wroom_32u
pio device list
pio run -e esp32_wroom_32u -t upload
pio device monitor -b 115200
```

PlatformIO selects the upload port; specify `--upload-port <port>` only after identifying the connected unit. No port is hard-coded. Close the serial monitor before running service tools. The owner receives `owner.json` through a private commissioning record or printed label. `service.json` remains with authorized service personnel.

Initialize blank measurement storage using the authorized procedure in [Factory commissioning](docs/FACTORY_COMMISSIONING.md). Normal firmware uploads preserve the filesystem. **Do not use `uploadfs` on a commissioned device**: it replaces measurement storage. Portal assets are embedded in the application, so a separate asset upload is unnecessary.

## Local configuration

After every restart of a commissioned unit, join `CWS-SETUP-<device suffix>` using its setup-network password and open **http://192.168.4.1**. Sign in as `admin` using the separate administrator password. The initial password must be replaced before connectivity settings are available.

The firmware closes the portal and AP after **300 monotonic seconds**. Refreshing the browser does not extend the window. Sensor acquisition, storage and STA/MQTT operation continue. Password hashing runs in a separate task.

Wi-Fi configuration supports scanning, manual SSID entry, DHCP and validated static IPv4 settings. MQTT configuration includes hostname, port, device credentials, public CA import and connection timeout. TLS and hostname verification are mandatory. The approved prefix is `cws/v1/devices`.

“Test connection” restores the previous settings after the trial. “Test & save changes” commits only a successful trial. A working Wi-Fi configuration can be saved while the backend is unavailable; the portal labels backend validation as pending. Empty replacement-password fields retain saved secrets; explicit clear controls remove them. Restarting opens the same workflow for subsequent credential replacement. [Portal](docs/LOCAL_CONFIGURATION_PORTAL.md), [authentication](docs/ADMINISTRATOR_AUTHENTICATION.md), [rollback](docs/CONFIGURATION_RECOVERY.md).

## Acquisition, calibration and offline operation

Default intervals are 60 seconds for acquisition and 300 seconds for upload and health reporting. Failed or uncalibrated readings are invalid and carry null engineering values. Soil output is a **relative moisture index**, not volumetric water content. VWC and battery percentage remain null.

Divider resistance values are deliberately unset. Battery and solar calibration require the installed divider values, a measured linear ADC range, and independent multimeter references. AM2301A offsets, both soil calibration orientations and power thresholds persist in NVS. [Sensor acquisition](docs/SENSOR_ACQUISITION.md) and [calibration](docs/CALIBRATION.md) define the procedures and limits.

LittleFS retains up to 256 telemetry/event records. Default retention protects existing unacknowledged records and drops new records when full, reporting the loss. An explicit rolling-retention policy can evict the oldest record. MQTT PUBACK alone never marks a record durably stored. The backend must opt into and send the application acknowledgement contract. [Offline storage](docs/OFFLINE_STORAGE.md).

LCD and encoder support default off. `config.patch` enables the installed profile; LCD probing uses only the configured PCF8574 address. Encoder presence is an explicit setting. The optional display provides three status pages with encoder navigation. Buzzer output defaults disabled pending driver/polarity verification. [Headless operation](docs/HEADLESS_OPERATION.md).

## Integration and project layout

The backend integration package includes [topics](docs/MQTT_TOPICS.md), [schemas](docs/TELEMETRY_SCHEMA.md), [97 command contracts](docs/COMMAND_REFERENCE.md), [registration and ACL requirements](docs/BACKEND_INTEGRATION.md), and non-secret [fixtures](examples/). Commands have a target ID, expiration, correlation ID and, for mutations, an expected revision. Destructive actions also require the device ID as confirmation.

`tools/mqtt_tool.py` provides TLS subscription, command publication, response inspection, an acknowledgement simulator, a file-based durable receiver and a live device integration runner. Its credentials come from environment variables; [examples/.env.example](examples/.env.example) contains names only.

| Directory | Responsibility |
|---|---|
| `src`, `include` | Entry point, version, bounds and build-generated assets |
| `lib/Core`, `lib/Board` | Shared validation/calculations and immutable PCB pins |
| `lib/Sensors`, `lib/Actuators`, `lib/Display` | Acquisition, indicators and optional controls |
| `lib/Configuration`, `lib/Security`, `lib/Storage` | NVS commits, authentication and LittleFS queue |
| `lib/Communication`, `lib/LocalWebServer` | Async MQTT/Wi-Fi and bounded local HTTP |
| `lib/System`, `lib/Diagnostics` | Scheduling, command dispatch, telemetry and faults |
| `data/portal`, `schemas`, `tools`, `test`, `docs` | Offline UI, integration contracts, verification and operations |

## Security and release limits

Credentials and generated secret headers are ignored at both repository and project scope. No production credentials are supplied in source. Broker ACLs form the remote command authorization boundary. The local HTTP service is restricted to its WPA2 setup interface, with strict origin/host checks, CSRF tokens, HttpOnly sessions and login throttling.

The standard Arduino build does **not** enable flash encryption or secure boot. Physical flash access can recover stored router/broker credentials and compiled setup material. Release commissioning must establish the physical-access threat model. No hardware upload, electrical measurement, field trial or live device/broker acceptance test has been performed during host development. [TLS security](docs/TLS_SECURITY.md), [deployment](docs/DEPLOYMENT.md), [verification report](docs/VERIFICATION_REPORT.md).
