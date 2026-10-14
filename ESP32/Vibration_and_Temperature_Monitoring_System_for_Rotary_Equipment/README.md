# Vibration and Temperature Monitoring System for Rotary Equipment

An ESP32-S3 predictive-maintenance prototype for a rotary asset. It measures two vibration points and one RTD temperature channel, calculates health/risk/trend indicators, presents the result locally, records history, and can remove a relay-controlled load when protection logic enters a fault state.

## Features

- Two ADXL345 accelerometers at I2C addresses `0x53` and `0x1D`, configured for ±16 g operation.
- MAX31865 three-wire RTD interface with 100 Ω nominal RTD and 430 Ω reference resistor in the current firmware.
- RMS vibration, X/Y/Z samples, baseline recalibration, temperature/vibration rates, risk score, health score, forecast, worst-metric label, and recommendation.
- Normal, warning, critical, and fault states; a fault trips the relay and activates the alarm path until it is explicitly cleared under valid conditions.
- 20×4 I2C LCD, rotary encoder, status LED, buzzer, LittleFS event/log storage, local access-point dashboard, and non-blocking service loop.

## Pin and bus mapping

| Function | ESP32-S3 GPIO | Notes |
| --- | ---: | --- |
| I2C SDA / SCL | 8 / 9 | LCD, ADXL345 devices, and I2C peripherals. |
| SPI CS / MOSI / SCK / MISO | 5 / 11 / 12 / 13 | Reserved interface pins. |
| Buzzer | 14 | Verify installed driver polarity. |
| Encoder CLK / DT / switch | 16 / 17 / 18 | Inputs use internal pull-ups. |
| Relay | 21 | Active-high in the current relay driver. |
| Status LED | 48 | Local status output. |
| UART0 TX / RX | 43 / 44 | Serial/programming interface. |
| Boot | 0 | Avoid external circuitry that blocks boot. |

The exact board mapping is maintained in `code/lib/Pins/Pins.cpp`; schematics, PCB, assets, and fabrication deliverables are under `hardware/` and `production/`.

## Firmware architecture

`src/main.cpp` initializes the board, I2C bus, sensors, control services, storage, web server, and LCD. Its cooperative loop updates the encoder, temperature/vibration acquisition, `maintenance_manager`, relay, buzzer, indicators, UI, storage, network server, errors, power service, and reset service in turn. `maintenance_manager` owns the health decision and exposes a snapshot to the display and HTTP layers; the relay module only enforces the requested/protective state.

Do not treat health scores or forecast minutes as a certified diagnostic. They are configurable engineering indicators and need asset-specific baselines, limits, sampling validation, and field correlation before they influence maintenance schedules or automatic shutdowns.

## Build and upload

Install PlatformIO Core and build from `code`:

```powershell
pio run -e esp32-s3
pio device list
pio run -e esp32-s3 -t upload
pio device monitor -b 115200
```

The `platformio.ini` environment targets `esp32-s3-devkitc-1`, Arduino, LittleFS, and the `huge_app.csv` partition layout. PlatformIO resolves LiquidCrystal_I2C, Adafruit ADXL345, and Adafruit MAX31865 dependencies. Validate the build and run available host tests before flashing hardware.

## Commissioning and use

1. Inspect the schematic and board revision; wire sensors with the equipment de-energized.
2. Confirm I2C addresses: one ADXL345 must use `0x53`, the other `0x1D`; resolve address/pull-up conflicts before power-up.
3. Validate the RTD wiring type and resistor values. If the installed sensor is not a 3-wire 100 Ω RTD with the stated reference resistor, update and test the firmware before use.
4. Flash the device, connect to its local dashboard, set temperature/vibration warning and fault thresholds, and establish vibration baselines while the machine is known healthy.
5. Exercise warning, critical, fault, relay trip, audible indication, dashboard, log storage, and clear-fault paths with a safe test setup.
6. Recalibrate after a sensor is moved, machine mounting changes, or the mechanical baseline is altered.

## Developer notes

| Path | Responsibility |
| --- | --- |
| `code/lib/Sensors/` | MAX31865 and dual-ADXL345 acquisition. |
| `code/lib/Control/maintenance_manager/` | State, scores, trend/forecast, thresholds, and protection decision. |
| `code/lib/Actuators/` | Relay and buzzer implementation. |
| `code/lib/Display/`, `Input/`, `Communication/` | LCD, encoder, LED, and local dashboard. |
| `code/lib/Storage/` | Persistent configuration, logs, and event history. |
| `code/lib/UI/test.html` | Browser-only UI preview; not device firmware. |

Keep the pin map single-sourced in `Pins.cpp`, preserve non-blocking loop behavior, and add a safe regression test whenever changing thresholds, trip logic, persistence, or the HTTP control interface.

## Security and safety

Store deployment Wi-Fi credentials, dashboard passwords, API keys, and certificates in an ignored local `.env`/configuration file—never in source or the README. Commit only a redacted `.env.example` when configuration is added. Replace development access-point credentials before deployment and keep the local configuration network off untrusted networks.

This prototype is not a functional-safety, machine-guarding, or certified protection device. A relay controlled by an ESP32 is not a substitute for safety-rated contactors, overload relays, emergency stops, guarding, lockout/tagout, or qualified electrical installation. Test on a safe, isolated setup before connecting to rotating machinery or power circuits.

# Author

Egbe Raymond

Electrical / Electronic Engineer
Embedded Systems Developer
Full Stack Developer

---


## License

Released under the [MIT License](LICENSE).
