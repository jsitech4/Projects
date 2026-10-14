# Dual Axis Solar Tracker

## Overview

This project is a Dual Axis Solar Tracking System built using the ESP32-WROOM-32UE microcontroller and an L298D motor driver module. The system automatically tracks the direction of sunlight using four LDR sensors and positions the solar panel for maximum light exposure using two DC motors.

The tracker also includes:
- Automatic night detection and sleep mode
- Rotary encoder based calibration and control
- Direction indication LEDs
- Manual movement mode
- Saved calibration settings using ESP32 Preferences (NVS)

The project is designed using a modular architecture for easier maintenance, debugging, scalability, and future upgrades.

---

# Features

- Dual axis solar tracking
- Automatic horizontal and vertical alignment
- Night detection using dedicated LDR
- Deep sleep power saving mode
- Rotary encoder controlled calibration
- Manual motor control mode
- Direction indication LEDs
- Saved calibration settings
- PWM motor speed control
- Modular PlatformIO structure

---

# Hardware Used

## Main Controller
- ESP32-WROOM-32UE

## Motor Driver
- L298D Dual H-Bridge Motor Driver

## Sensors
- 4 × LDRs for sun tracking
- 1 × LDR for night detection

## Inputs
- Rotary Encoder with push button

## Indicators
- 5 mm LEDs for direction indication

## Motors
- 2 × DC Gear Motors
  - Azimuth movement
  - Elevation movement

---

# Pin Configuration

## Motor Driver

| Signal | GPIO |
|---|---|
| AZ_EN | GPIO25 |
| AZ_IN1 | GPIO32 |
| AZ_IN2 | GPIO33 |
| EL_EN | GPIO14 |
| EL_IN1 | GPIO26 |
| EL_IN2 | GPIO27 |

## LDR Sensors

| Sensor | GPIO |
|---|---|
| Top Left | GPIO4 |
| Top Right | GPIO35 |
| Bottom Left | GPIO39 |
| Bottom Right | GPIO34 |
| Night LDR | GPIO36 |

## Rotary Encoder

| Signal | GPIO |
|---|---|
| CLK | GPIO18 |
| DT | GPIO19 |
| SW | GPIO23 |

## LEDs

| LED | GPIO |
|---|---|
| LEFT | GPIO5 |
| RIGHT | GPIO17 |
| UP | GPIO16 |
| DOWN | GPIO15 |
| STATUS | GPIO2 |

---

# LDR Wiring

3.3V ---- LDR ---- ADC PIN ---- 10k Resistor ---- GND

The ADC pin must be connected at the center point between the LDR and resistor.

---

# Project Structure

```text
lib/
├── Control/
├── Display/
├── Drivers/
├── Input/
├── Pins/
├── Sensors/
├── System/

src/
└── main.cpp
```

---

# Rotary Encoder Operation

## Short Press
Cycles through:
- AUTO_TRACK
- MANUAL_AZIMUTH
- MANUAL_ELEVATION
- CALIBRATE_DEADBAND
- CALIBRATE_NIGHT

## Long Press
Returns system to AUTO_TRACK mode.

## Rotation Functions

### MANUAL_AZIMUTH
Moves panel left and right.

### MANUAL_ELEVATION
Moves panel up and down.

### CALIBRATE_DEADBAND
Adjusts tracking sensitivity.

### CALIBRATE_NIGHT
Adjusts night detection threshold.

---

# Operating Guide

## Power On

1. Power the ESP32 and L298D driver.
2. Ensure all LDRs are exposed to light.
3. The status LED should begin breathing.
4. The system starts in AUTO_TRACK mode.

---

## Automatic Tracking

The system compares:
- Left vs Right light intensity
- Top vs Bottom light intensity

The motors automatically reposition the solar panel toward the strongest light source.

---

## Manual Control

Use short press to enter:
- MANUAL_AZIMUTH
- MANUAL_ELEVATION

Rotate the encoder to move the panel manually.

---

## Calibration

### Deadband Calibration

Higher deadband:
- Less motor movement
- Less jitter

Lower deadband:
- Faster response
- Higher sensitivity

### Night Threshold Calibration

Adjusts the darkness level required before entering sleep mode.

---

# Sleep Mode

When darkness is detected:
- Motors stop
- LEDs turn off
- ESP32 enters deep sleep

Wake sources:
- Rotary encoder button
- Timer wakeup

---

# PWM Motor Control

PWM is used on:
- GPIO25
- GPIO14

This provides:
- Smooth movement
- Adjustable speed
- Reduced motor stress

---

# Important Notes

- Use a separate motor power supply.
- Do not power motors directly from the ESP32.
- All grounds must be connected together.
- Use voltage dividers for all LDRs.
- GPIO34–39 are input-only pins.
- GPIO4 is ADC2 and may conflict with WiFi usage.

---

# PlatformIO Environment

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

---

# Uploading

## Build

```bash
pio run
```

## Upload

```bash
pio run --target upload
```

## Serial Monitor

```bash
pio device monitor
```

---

# Security Note

Sensitive information such as:
- WiFi password
- Access point password
- API keys

have intentionally been hidden using:

*****
before publishing to GitHub.

---

# Future Improvements

- Astronomical tracking
- OTA updates
- Web dashboard
- Battery monitoring
- MPPT integration
- Weather integration
- Limit switches

---

## Security and local configuration

Never commit populated `.env` files, Wi-Fi credentials, passwords, API keys, SMTP credentials, broker tokens, private certificates, or production connection strings. Keep them in an ignored local configuration file or approved secret store, and commit only a redacted `.env.example` when configuration is needed. Replace development access-point credentials before deployment and limit local administration to trusted networks.

# Author

Egbe Raymond

Electrical / Electronic Engineer  
Embedded Systems Developer  
Full Stack Developer

---

## Engineering asset map and license scope

The editable electrical design is in `hardware/` as the KiCad project,
schematic, and PCB. Supporting symbols, footprints, 3D models, source/exported
documentation, rendered images, the BOM, Gerbers, IPC netlist, designators,
and placement data are included under `hardware/`, `documentation/`, and
`production/`. Treat the KiCad source as authoritative; regenerate production
files after a design change instead of editing Gerbers.

The PlatformIO firmware in `code/` targets `esp32dev` with Arduino at 115200
baud. Its modules separate sun-sensor processing, L298D motor driving, rotary
encoder input, LEDs, error handling, reset, and sleep/wake control. The fixed
GPIO map and motor-power cautions above must be reviewed against the schematic
before wiring a panel or motor supply.

The project's original firmware, KiCad design, original library assets, BOM,
exports, images, and documentation are licensed by asset type below. Component datasheets,
vendor models, PlatformIO libraries, and manufactured parts are not relicensed;
their original terms still apply. Read the [license scope guide](../../LICENSE-SCOPE.md)
for the practical boundary and safety responsibilities.

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
