# Hot Liquid Monitoring and Control System

ESP32-S3 firmware and KiCad hardware for monitoring liquid temperature and vessel level, presenting live status on a local dashboard/LCD, recording events, and driving a protected relay output according to configurable level and temperature rules.

## What it does

- Measures temperature through a MAX31865 RTD interface and liquid distance/level through an ultrasonic sensor.
- Converts distance to a configurable percentage level and classifies low/normal/full tank states.
- Classifies temperature against configurable low/high limits.
- Combines independent automatic demands with a persistent manual relay request; the effective relay request is `manual OR automatic`.
- Provides latching modes for tank and temperature events, a buzzer/LED indication path, local web dashboard, LCD output, internal flash settings/event storage, sleep/wake service, and reset handling.

## Hardware mapping

| Function | ESP32-S3 GPIO | Notes |
| --- | ---: | --- |
| I2C SDA / SCL | 8 / 9 | LCD and I2C peripherals. |
| ZMCT103C ADC | 1 | Defined on the PCB pin map; verify use before enabling. |
| ADXL345 interrupt | 10 | Defined on the PCB pin map; not part of the primary control loop. |
| DS18B20 data | 4 | Defined on the PCB pin map. |
| SPI CS / MOSI / SCK / MISO | 5 / 11 / 12 / 13 | Reserved interface pins. |
| Ultrasonic trigger / echo | 17 / 18 | Level sensor; condition echo voltage for 3.3 V logic. |
| Buzzer | 14 | Active level must match the installed driver. |
| Relay | 21 | Verify relay polarity and load isolation before use. |
| Status LED | 48 | Local state indication. |
| UART0 TX / RX | 43 / 44 | Serial/programming interface. |
| Boot | 0 | Do not load in a way that prevents boot. |

The deployed hardware and source of truth are in `hardware/` and `code/lib/Pins/Pins.cpp`. Review the KiCad schematic before connecting sensors or mains-powered equipment.

## Operating model

At startup the firmware restores a validated saved configuration or retains safe defaults. Each loop updates the RTD and ultrasonic sensors, evaluates `device_manager`, applies the relay request, updates buzzer/LED/LCD/storage/web services, and yields. A latched automatic threshold event cannot be cleared merely by changing settings: `clearFault()` rechecks valid, current inputs and clears only when all enabled threshold conditions have recovered. Manual relay requests remain separate from faults.

The local web UI exposes live state, configuration, event records, manual relay control, and guarded fault clearing. Treat browser controls as an operator interface—not as a substitute for thermal cut-outs, pressure relief, contactors, or independent safety interlocks.

## Build, upload, and test

Prerequisites: PlatformIO Core, an ESP32-S3 DevKitC-1, and the components stated in `platformio.ini` (LiquidCrystal_I2C, Adafruit ADXL345, and Adafruit MAX31865).

```powershell
cd code
pio run -e esp32-s3
pio device list
pio run -e esp32-s3 -t upload
pio device monitor -b 115200
```

Run the host fault tests from `code` using the project’s test configuration. Before energizing a live actuator, test with the relay load disconnected, verify the output polarity, calibrate full/low vessel distances, validate temperature limits with a reference thermometer, and confirm the relay remains de-energized on reset and sensor error.

## Configuration and usage

1. Wire only isolated, low-voltage sensor signals. The ultrasonic echo input must be reduced to 3.3 V logic.
2. Flash the firmware and connect to the device’s local access point/dashboard as reported by the firmware.
3. Set full and low distances, level thresholds, temperature limits, and the intended latch modes.
4. Verify level conversion with known vessel levels; verify the RTD reading against a trusted thermometer.
5. Test automatic demand, manual demand, latch clearing, buzzer/LED, dashboard, and persistence with no hazardous load attached.
6. Connect the final relay through a correctly rated, isolated driver and independently verify fail-safe behavior.

## Project layout

| Path | Purpose |
| --- | --- |
| `hardware/` | KiCad project, symbols, footprints, 3D models, and PCB design. |
| `code/src/` | Firmware entry point and initialization order. |
| `code/lib/Control/` | Control state, fault logic, sleep/reset, and device manager. |
| `code/lib/Sensors/` | RTD and ultrasonic acquisition. |
| `code/lib/Actuators/`, `Display/`, `Communication/` | Relay, buzzer, UI, and local server. |
| `code/lib/Storage/` | Persistent settings and event history. |
| `production/` | BOM, fabrication, and Gerber deliverables. |

## Security and secrets

Do not place Wi-Fi credentials, local dashboard passwords, cloud/API tokens, or private certificates in committed source. Store them in a local `.env`/ignored configuration file and inject them during the build or provisioning process; provide only a redacted `.env.example` when adding configuration. Change any development access-point password before deployment, restrict the configuration network to trusted users, and do not expose an unauthenticated control UI to the internet.

## Safety

Hot liquid, heaters, pumps, and mains equipment can cause burns, fire, electrocution, or pressure hazards. This project is not a certified safety controller. Use a qualified electrician, fused and grounded hardware, rated contactors/SSR drivers, a mechanical thermal cut-out, independent over-temperature and dry-run protection, emergency isolation, and local regulatory requirements. Never rely on firmware alone to protect people or equipment.

# Author

Egbe Raymond

Electrical / Electronic Engineer
Embedded Systems Developer
Full Stack Developer

---


## License

Released under the [MIT License](LICENSE).
