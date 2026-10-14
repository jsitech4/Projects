# Crop Water-Stress Risk Monitoring Platform

An end-to-end, self-hosted crop-monitoring platform for an ESP32-WROOM-32U field node. The device acquires ambient temperature/humidity, relative soil moisture, battery voltage, and solar voltage; retains observations while offline; and delivers telemetry to a TLS-protected MQTT backend. The repository also contains the API, operator dashboard, local service stack, and a hardware-faithful MQTT simulator.

> **Scope.** Soil output is a *relative moisture index*, not a calibrated volumetric-water-content measurement. The ESP32 connects to a 2.4 GHz Wi-Fi router; any 4G/5G uplink is provided by the router, not by the device.

## System capabilities

- AM2301A environmental acquisition, soil/battery/solar ADC acquisition, calibration, validity checks, and threshold evaluation.
- Local first-time provisioning portal with authenticated administration, connection trials, CSRF protection, and a five-minute setup window.
- MQTT over verified TLS, versioned JSON contracts, per-device topics, command correlation, and application-level acknowledgements.
- Durable LittleFS queue (up to 256 telemetry/event records) with a selectable full-queue policy and recovery after reconnect.
- Optional LCD2004 and rotary encoder profile; the default build is headless.
- Fastify/Prisma API, Next.js dashboard, PostgreSQL, Mosquitto development stack, and simulator scenarios for normal operation, dry stress, sensor fault, and offline recovery.

## Hardware and fixed GPIO mapping

| Function | GPIO | Notes |
| --- | ---: | --- |
| AM2301A data | 32 | Digital temperature/humidity input. |
| Soil ADC | 34 | ADC1 input; use a calibrated, suitably conditioned sensor. |
| Battery ADC | 35 | ADC1 input; never exceed ESP32 ADC limits. |
| Solar ADC | 39 | ADC1 input; requires a correctly designed divider. |
| I2C SDA / SCL | 21 / 22 | Optional LCD2004 profile. |
| Encoder CLK / DT / switch | 25 / 26 / 27 | Optional local navigation. |
| Status LED | 2 | Device status indication. |
| Buzzer | 4 | Disabled by default until driver polarity is verified. |

GPIO14, GPIO12, GPIO33, and GPIO16 are intentionally not controlled by this application. Consult `code/hardware/docs/HARDWARE.md`, `PIN_MAPPING.md`, and the KiCad files before changing the PCB or wiring.

## Repository layout

| Path | Purpose |
| --- | --- |
| `hardware/` | KiCad schematic, PCB, symbols, footprints, 3D models, fabrication files, BOM, and rendered documentation. |
| `code/hardware/` | PlatformIO firmware, portal assets, JSON schemas, tests, tools, and operational references. |
| `code/software/` | Self-hosted API, web dashboard, database schema, contracts, deployment material, and Docker services. |
| `code/simulation/` | MQTT device simulator for end-to-end development without an ESP32. |
| `production/` | Gerber, fabrication, and BOM deliverables where provided. |

## Quick start

### Firmware

Install PlatformIO Core, then work from `code/hardware`:

```powershell
pio run -e esp32_wroom_32u
pio test -e native
pio device list
pio run -e esp32_wroom_32u -t upload
pio device monitor -b 115200
```

Native tests require a C++17 `g++`. Optional Python verification tools are installed with `python -m pip install -r tools/requirements.txt`; run `python tools/test_persistence.py` and `python -m pytest test/integration -q` as applicable. Do not use `uploadfs` on a commissioned device: it replaces LittleFS measurement storage.

### Backend and dashboard

From `code/software`, copy `.env.example` to `.env`, set local development values, then run:

```powershell
npm install
npm run db:generate
npm run db:migrate
npm run dev
```

Use `npm run build`, `npm test`, `npm run lint`, and `npm run typecheck` before deployment. `docker-compose.yml` is for local development; use TLS, distinct service accounts, ACLs, HTTPS, and separate staging/production infrastructure for a real deployment.

### Simulator

Start the local services first. In `code/simulation`, copy `.env.example` to `.env`, install dependencies, then run one scenario:

```powershell
npm install
npm run normal
# or: npm run dry-stress | npm run sensor-fault | npm run offline-recovery
```

The simulator registers a local demo device and publishes contract-shaped MQTT messages. It is a development aid, not an electrical model of the ESP32.

## Commissioning and operation

1. Generate a unique private factory record: `python tools/commission.py create --directory provisioning/private/unit-001`.
2. Set `CWS_FACTORY_FILE` only in the local shell, build, upload, and connect to the temporary `CWS-SETUP-<suffix>` access point.
3. Open `http://192.168.4.1`, sign in, replace the initial administrator password, and configure 2.4 GHz Wi-Fi.
4. Import the public CA, configure the device-specific MQTT identity, and use **Test & save changes**. The firmware retains prior settings after a failed trial.
5. Calibrate the divider and soil sensor using independent multimeter/reference measurements. Verify retention, backend acknowledgement, and telemetry before field deployment.

Default acquisition is 60 seconds; upload and health reporting default to 300 seconds. The backend must send the documented application acknowledgement—MQTT PUBACK alone does not mark a record safely delivered.

## Security and local secrets

Never commit `.env`, factory records, private keys, certificates, router credentials, broker passwords, database URLs, session secrets, encryption keys, or device owner/service credentials. The repository ignores these files and provides `code/software/.env.example`, `code/simulation/.env.example`, and `code/hardware/examples/.env.example` as variable-name templates.

At minimum, production configuration requires `DATABASE_URL`, `MQTT_BROKER_URL`, `MQTT_USERNAME`, `MQTT_PASSWORD`, one CA source (`MQTT_CA_PATH` or `MQTT_CA_PEM`), `SESSION_SECRET`, and `ENCRYPTION_KEY`. Use unique high-entropy values, least-privilege per-device MQTT credentials, broker ACLs under `cws/v1/devices`, and secret rotation. The standard Arduino build does not enable secure boot or flash encryption; account for physical access in the deployment threat model.

## Developer guide

- Firmware boundaries are documented in `code/hardware/docs/`: hardware, acquisition, calibration, portal, MQTT topics/configuration, schemas, command reference, persistence, testing, and deployment.
- Keep contracts versioned and validate fixtures with `tools/validate_schema.py`; commands must carry target, expiry, correlation, and expected revision where required.
- Do not repurpose the fixed pin mapping or introduce uncontrolled GPIO commands. Validate on isolated, low-risk hardware first.
- Preserve storage compatibility and test power-loss/recovery paths when changing configuration or queue code.
- Make UI changes in `data/portal`, then rebuild assets with the supplied tooling and run the browser/integration tests.

## Safety and limitations

This is an engineering project, not a certified agricultural-control or safety system. Use protected power inputs, correctly rated dividers, weatherproof enclosures, suitable fusing, and independent failsafes. Validate sensor calibration, battery protection, irrigation/control decisions, and backend alerting under the actual field conditions before relying on the system.

# Author

Egbe Raymond

Electrical / Electronic Engineer
Embedded Systems Developer
Full Stack Developer

---


## Engineering asset map and license scope

This is the collection's full-stack project. `code/hardware/` contains the
ESP32-WROOM-32U PlatformIO firmware, portal assets, schemas, tests, tools, and
hardware-specific commissioning notes. `code/software/` contains the Fastify
API, Prisma database schema, Next.js operator interface, shared contracts,
Mosquitto/Caddy deployment examples, and Docker development stack.
`code/simulation/` is a separate MQTT field-node simulator used to exercise
the API without physical hardware. The documentation folder includes the
ESP32 reference PDF; generated dependency folders are not source material.

Original firmware, application code, schemas, deployment configuration, and
documentation contributed here are covered by the asset-type licenses below.
This project does not currently ship a top-level KiCad design or fabrication
package, so no such asset is implied by this README. Datasheets, npm packages,
container images, MQTT software, and other third-party dependencies keep their
own terms. See the collection-wide [license scope guide](../../LICENSE-SCOPE.md)
before reusing or redistributing a deployment.

## Licenses

The original JSI Tech material in this software and documentation project is
licensed by asset type:

| Material | License | Scope |
| --- | --- | --- |
| Software and firmware | [MIT](LICENSE) | Original ESP32 firmware, API, web application, simulator, scripts, configuration examples, tests, and deployment assets in `code/`. |
| Documentation | [CC BY 4.0](LICENSE-DOCUMENTATION) | Original README text, written documentation, diagrams, photos, and screenshots. |
| External material | [Third-party notices](THIRD-PARTY-NOTICES.md) | Datasheets, dependencies, container images, broker/database software, and other material not created by JSI Tech. |

This project has no top-level KiCad design or production-hardware source and
therefore no hardware-design license notice. The [collection licensing guide](../../LICENSE-SCOPE.md)
explains the boundary and limitations.
