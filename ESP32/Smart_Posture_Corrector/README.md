# Smart Posture Corrector

## Overview

This project is a modular Smart Posture Corrector System built around the ESP32-WROOM-32 using a structured PlatformIO architecture.

The system monitors the user posture orientation using the MPU6050 gyroscope and accelerometer sensor and intelligently detects bad posture based on a calibrated body reference angle.

When poor posture is maintained for a configured duration, the system activates:

- Vibration motor feedback
- Audible buzzer alert
- LED fault indication

The system also supports:

- Persistent posture calibration storage
- Battery monitoring
- SD card logging
- Sleep/wake power management
- Breathing heartbeat LED effect
- Non-blocking firmware design

The posture calibration is automatically saved using ESP32 Preferences memory so the device remembers the user's posture reference angle after restart or power loss.

---

# Features

- ESP32-WROOM-32 based controller
- MPU6050 posture monitoring
- Real-time pitch and roll calculation
- Intelligent posture analysis
- Automatic bad posture detection
- Vibration motor feedback alert
- Buzzer posture alert
- Battery level monitoring
- Four-level battery LED indicator
- Breathing heartbeat LED effect
- SD card posture logging
- Saved calibration reference using Preferences memory
- Short press mute/unmute function
- Long press recalibration function
- Deep sleep power saving support
- Non-blocking firmware architecture
- Modular PlatformIO project structure

---

# Hardware Used

## Main Controller

- ESP32-WROOM-32 development board

## Motion Sensor

- MPU6050 Gyroscope + Accelerometer

## Storage

- MicroSD Card Module

## User Input

- Push button

## Alert System

- Vibration motor with optocoupler driver
- Buzzer with optocoupler driver

## Battery Monitoring

- Voltage divider circuit
- Four battery level LEDs

---

# Pin Configuration

| Function | GPIO |
|---|---|
| MPU6050 SDA | GPIO21 |
| MPU6050 SCL | GPIO22 |
| MPU6050 INT | GPIO27 |
| SD Card CS | GPIO5 |
| SD Card MOSI | GPIO23 |
| SD Card MISO | GPIO19 |
| SD Card SCK | GPIO18 |
| Vibration Motor | GPIO25 |
| Buzzer | GPIO26 |
| Battery ADC | GPIO34 |
| LED 1 | GPIO13 |
| LED 2 | GPIO14 |
| LED 3 | GPIO16 |
| LED 4 | GPIO17 |
| User Button | GPIO32 |

---

# PlatformIO Environment

```ini
[platformio]
description = Smart_Posture_Corrector

[env:esp32dev]
platform = espressif32@6.5.0
board = esp32dev
framework = arduino
monitor_speed = 115200
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

The editable board design is in `hardware/` as the KiCad project, schematic,
and PCB. `documentation/` provides the block/schematic PDFs, ESP32 reference,
board renders, layout images, and photographs; `production/` supplies the BOM,
Gerbers, positions, designators, and IPC netlist. Use the KiCad files as the
source of truth and regenerate manufacturing exports after any revision.

`code/` builds as Arduino firmware for `esp32dev` at 115200 baud. The modules
separate MPU6050 acquisition, posture logic, button input, vibration motor and
buzzer outputs, LED status, battery measurement, SD storage, Wi-Fi/local
server support, reset, error handling, and sleep/wake control. Fit, sensor
orientation, thresholds, vibration intensity, battery protection, and user
comfort all require physical validation for the intended wearable enclosure.

The asset-type table below covers the original firmware, KiCad design, original project exports,
images, and documentation. Sensor/ESP32 vendor files, libraries, external
models, and trademarks retain their own terms. The [license scope guide](../../LICENSE-SCOPE.md)
also makes clear that this is not medical advice or a medical-device approval.

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

