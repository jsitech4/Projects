# Smart Transformer Health Monitoring System

This project is an ESP32-S3 based smart transformer health monitoring system. It monitors transformer temperature, load current, AC voltage, vibration, relay status, storage status, and predictive maintenance condition from a local web dashboard.

## Main Features

- Smart transformer health monitoring with temperature, current, voltage, and vibration analysis
- Predictive maintenance risk score and transformer health score
- Future fault estimate based on changing temperature, current, voltage, and vibration trends
- Local web dashboard hosted directly by the ESP32-S3
- SD card backend for transformer logs, analysis logs, and event logs
- Internal LittleFS fallback when SD card is not available
- Relay protection output for disconnecting the transformer load during fault condition
- LCD display pages for live data, health status, future analysis, backend status, and ADC readings
- Dark and light dashboard theme with icon toggle

## Important Safety Note

The ESP32 ADC pin must never be connected directly to AC mains or a high-voltage transformer terminal. The voltage sensing input on GPIO2 must receive only a properly isolated, scaled, and biased low-voltage waveform within the ESP32 ADC range. For mains or high-voltage transformer measurement, use a proper isolated voltage sensing module such as a ZMPT101B-type module or another certified isolated voltage transducer.

## Main Pin Assignment

| Function                   | ESP32-S3 GPIO |
| -------------------------- | ------------: |
| I2C SDA                    |         GPIO8 |
| I2C SCL                    |         GPIO9 |
| ADXL345 INT                |        GPIO10 |
| ZMCT103C current ADC       |         GPIO1 |
| Transformer AC voltage ADC |         GPIO2 |
| DS18B20 data               |         GPIO4 |
| SD CS                      |         GPIO5 |
| SD MOSI                    |        GPIO11 |
| SD SCK                     |        GPIO12 |
| SD MISO                    |        GPIO13 |
| Buzzer                     |        GPIO14 |
| Rotary encoder CLK         |        GPIO16 |
| Rotary encoder DT          |        GPIO17 |
| Rotary encoder SW          |        GPIO18 |
| Transformer relay          |        GPIO21 |
| Status LED                 |        GPIO48 |

## Voltage Measurement Input

GPIO2 has been assigned as the free ADC pin for transformer AC voltage sensing.

The new module is located at:

```text
lib/Power/voltage_sensor/voltage_sensor.h
lib/Power/voltage_sensor/voltage_sensor.cpp
```

The firmware measures the AC component by sampling the ADC waveform, removing the DC bias mathematically, calculating RMS at the ADC pin, and multiplying it by the voltage calibration factor.

Default calibration value:

```cpp
static float sensorCalibration = 100.0f;
```

Adjust this value after comparing the dashboard reading with a trusted meter reading.

## Web Dashboard

Connect to the ESP32 access point:

```text
SSID: TRANSFORMER_HM_SYSTEM
Password: 12345678
```

Then open:

```text
http://192.168.4.1
```

The dashboard shows:

- Transformer temperature
- Transformer AC voltage
- Transformer current
- Transformer vibration RMS
- Risk score
- Health score
- Future fault estimate
- Worst metric
- Recommendation
- Relay control
- Recent transformer data log
- Recent analysis log
- Recent event log

## Storage Files

The project now stores transformer-related logs for transformer health history.

| File                   | Purpose                                              |
| ---------------------- | ---------------------------------------------------- |
| `/transformer_log.csv` | Main transformer sensor and condition data           |
| `/analysis_log.csv`    | Predictive maintenance analysis and recommendations  |
| `/event_log.csv`       | Boot, relay, fault, storage, and state-change events |

## Predictive Logic

The transformer health logic uses:

- Temperature severity
- Current severity
- Voltage abnormality severity
- Vibration severity
- Trend rate per minute
- Estimated time-to-fault

Default voltage limits are configured for a 230 V class transformer output/input reading:

| Limit        | Default |
| ------------ | ------: |
| Low warning  |   180 V |
| Low fault    |   160 V |
| High warning |   250 V |
| High fault   |   270 V |

These values should be adjusted if the monitored transformer voltage is not a 230 V class AC voltage.

## Build and Upload

Open the `code` folder in VS Code with PlatformIO, then build and upload using the environment:

```ini
[env:esp32-s3-wroom-1u]
```

Serial monitor speed:

```text
115200 baud
```

## Security and local configuration

Never commit populated `.env` files, Wi-Fi credentials, passwords, API keys, SMTP credentials, broker tokens, private certificates, or production connection strings. Keep them in an ignored local configuration file or approved secret store, and commit only a redacted `.env.example` when configuration is needed. Replace development access-point credentials before deployment and limit local administration to trusted networks.

# Author

Egbe Raymond

Electrical / Electronic Engineer
Embedded Systems Developer
Full Stack Developer

---

## Engineering asset map and license scope

The `hardware/` folder contains the editable KiCad PCB, schematic, and project
along with source library and 3D assets. `documentation/` contains the ESP32-S3
reference and board images; `production/` provides Gerbers, BOM, positions,
designators, and IPC netlist for manufacturing. Use the KiCad project to review
the design and regenerate exports after an approved modification.

`code/` is Arduino/PlatformIO firmware for `esp32-s3-devkitm-1` at 115200 baud.
It separates vibration and temperature sensing, voltage/current measurement,
load relay and buzzer control, LCD/LED UI, rotary input, SD-card storage, local
server, maintenance management, reset, error handling, and sleep/wake logic.
Measurements and thresholds support condition monitoring only; protection of a
transformer requires independently engineered sensing, isolation, and trip
systems appropriate to the installation.

The asset-type table below applies to original project source, KiCad design material, original exports,
images, and documentation. Vendor references, third-party libraries/models,
components, and trademarks keep their own conditions. Review the
[license scope guide](../../LICENSE-SCOPE.md) and never treat it as a high-
voltage safety certification.

## Licenses

The original JSI Tech material in this mixed engineering project is licensed by
asset type:

| Material | License | Scope |
| --- | --- | --- |
| Software and firmware | [MIT](LICENSE) | Original source code, scripts, configuration examples, tests, and web assets in `code/`. |
| Hardware design | [CERN-OHL-P-2.0](LICENSE-HARDWARE) | Original KiCad/design and manufacturing material in `hardware/` and `production/`. |
| Documentation | [CC BY 4.0](LICENSE-DOCUMENTATION) | Original README text, written documentation, diagrams, photos, and screenshots. |
| External material | [Third-party notices](THIRD-PARTY-NOTICES.md) | Vendor data, third-party libraries/models, dependencies, logos, and other material not created by JSI Tech. |

The [collection licensing guide](../../LICENSE-SCOPE.md) explains the boundary,
required notices, and limitations.
