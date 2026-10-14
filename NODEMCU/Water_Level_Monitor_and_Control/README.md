# Water Level Monitoring and Automatic Pump Control System

## Overview

This project is a NodeMCU based Water Level Monitoring and Automatic Pump Control System designed to monitor the water level inside a tank using an ultrasonic sensor, display the water level using an LED bar graph through the PCA9555D I2C I/O expander, and automatically control a water pump based on the tank level.

The system continuously measures the distance between the ultrasonic sensor and the water surface, converts the reading into a percentage value, and updates the LED graph accordingly.

The pump automatically turns ON when the water level falls below 20% and turns OFF once the tank reaches 100%.

The project includes:

- NodeMCU ESP8266 controller
- Ultrasonic water level sensing
- Automatic water pump control
- PCA9555D LED bar graph display
- Real-time percentage monitoring
- Serial Monitor debugging output
- I2C communication
- Tank percentage mapping logic

---

# Features

- Real-time ultrasonic water level measurement
- Water level percentage calculation
- LED bar graph display using PCA9555D
- Automatic pump ON/OFF control
- Pump ON below 20%
- Pump OFF at 100%
- Serial Monitor live debugging
- Stable I2C communication
- Expandable design structure

---

# Hardware Used

## Main Controller

- NodeMCU ESP8266

## Sensors

- HC-SR04 Ultrasonic Sensor

## Output Devices

- Relay Module
- Water Pump
- LED Bar Graph

## Communication

- PCA9555D I2C I/O Expander

---

# Pin Configuration

## Ultrasonic Sensor

| Signal | NodeMCU Pin |
| ------ | ------------ |
| TRIG   | D6 |
| ECHO   | D5 |

---

## Pump Relay

| Device | NodeMCU Pin |
| ------ | ------------ |
| Relay Input | D7 |

---

## PCA9555D I2C

| Signal | NodeMCU Pin |
| ------ | ------------ |
| SDA | D2 |
| SCL | D1 |

---

# LED Bar Graph Mapping

| Water Level | PCA9555 Pin |
| ------------ | ------------ |
| 100% | 101_0 |
| 90% | 101_1 |
| 80% | 101_2 |
| 70% | 100_0 |
| 60% | 100_1 |
| 50% | 100_2 |
| 40% | 100_3 |
| 30% | 100_4 |
| 20% | 100_5 |
| 10% | 100_6 |
| 0% | 100_7 |

---

# System Operation

## General Workflow

1. The ultrasonic sensor continuously measures the water distance.
2. The measured distance is converted into water level percentage.
3. The LED graph updates according to the water level.
4. When the level drops below 20%, the relay activates the pump.
5. The pump continues filling the tank.
6. Once the tank reaches 100%, the pump turns OFF automatically.
7. The Serial Monitor displays:
   - Distance in centimeters
   - Water percentage
   - Pump status

---

# Water Level Logic

The system maps the measured ultrasonic distance into percentage values.

## Tank Logic

- Smaller distance = Higher water level
- Larger distance = Lower water level

Example:

| Distance | Water Level |
| -------- | ------------ |
| 5 cm | 100% |
| 25 cm | 50% |
| 50 cm | 0% |

---

# Pump Logic

## Pump ON Condition

The pump turns ON when:

```text
Water Level <= 20%
```

## Pump OFF Condition

The pump turns OFF when:

```text
Water Level >= 100%
```

This prevents:

- Tank overflow
- Dry running conditions
- Constant relay switching

---

# LED Graph Operation

The LEDs operate as a bar graph display.

## Behavior

- Low water level lights fewer LEDs
- High water level lights more LEDs
- Full tank lights all LEDs
- Empty tank lights only the lowest level indicator

This allows quick visual monitoring of tank level.

---

# Serial Monitor Output

The Serial Monitor displays live information.

Example:

```text
Distance: 18 cm | Level: 64% | Pump: ON
```

This helps during:

- Calibration
- Troubleshooting
- Tank testing
- Sensor verification

---

# Software Requirements

The project uses Arduino framework with PlatformIO.

## Required Libraries

- Wire
- PCA95x5

---

# PlatformIO Configuration

Example `platformio.ini`:

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino

lib_deps =
    hideakitai/PCA95x5@^0.1.3
```

---

# Running the Project

## Step 1 — Connect Hardware

Connect:

- Ultrasonic sensor
- PCA9555D
- Relay module
- Pump
- LEDs

according to the pin configuration.

---

## Step 2 — Build the Project

```bash
pio run
```

---

## Step 3 — Upload the Firmware

```bash
pio run --target upload
```

---

## Step 4 — Open Serial Monitor

```bash
pio device monitor
```

Baud rate:

```text
9600
```

---

# Operating Guide

## Normal Operation

1. Power the NodeMCU.
2. The ultrasonic sensor begins reading the tank level.
3. The LED graph updates automatically.
4. If the tank level drops below 20%, the pump starts automatically.
5. When the tank becomes full, the pump stops automatically.
6. The Serial Monitor continuously displays system information.

---

# Troubleshooting

## Ultrasonic Read Error

Possible causes:

- Sensor wiring issue
- Object outside detection range
- Sensor facing incorrect direction
- Weak power supply

---

## LEDs Not Responding

- Check PCA9555D I2C wiring
- Confirm I2C address is `0x20`
- Verify SDA and SCL connections
- Check LED polarity

---

## Pump Not Turning ON

- Confirm relay wiring
- Check pump power supply
- Verify relay trigger logic
- Confirm water level is below 20%

---

## Pump Not Turning OFF

- Confirm ultrasonic readings
- Check water level mapping values
- Verify tank depth configuration

---

# Safety Notes

- Use a proper power supply for the pump
- Isolate relay high-voltage wiring
- Avoid water contact with electronics
- Secure all I2C and sensor wiring
- Use proper grounding

---

# Future Improvements

- OLED or LCD display support
- Wi-Fi monitoring dashboard
- Blynk integration
- Mobile notifications
- EEPROM configuration storage
- Multiple tank monitoring
- Battery backup support
- Dry-run pump protection

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

The editable hardware design is contained in `hardware/` as the KiCad project,
PCB, and schematic, with custom NodeMCU/HC-SR04 symbols, footprints, and STEP
models. `documentation/` includes the BOM document, schematic PDF, top/bottom
PCB-layer PDFs, and board images. No `production/` release folder is supplied;
use the KiCad sources to generate Gerbers, drill files, positions, and a new
BOM for the exact board revision you intend to fabricate.

`code/` is a focused Arduino PlatformIO project for `nodemcuv2`; its
`src/main.cpp` reads the HC-SR04, maps tank height to percent, drives a relay,
and updates the PCA9555D I2C LED bar graph. Build/upload from `code/` with
`pio run`, `pio run -t upload`, and `pio device monitor` after confirming the
real tank's empty/full distances and the relay's active polarity.

Original firmware, KiCad designs, original 3D assets, documentation, images,
and BOM material are covered by the asset-type licenses below. Module datasheets,
third-party libraries, and vendor component models retain their own terms. The
[license scope guide](../../LICENSE-SCOPE.md) also explains that the license
does not guarantee pump safety, water isolation, or electrical compliance.

## Licenses

The original JSI Tech material in this mixed engineering project is licensed by
asset type:

| Material | License | Scope |
| --- | --- | --- |
| Software and firmware | [MIT](LICENSE) | Original source code, scripts, configuration examples, tests, and web assets in `code/`. |
| Hardware design | [CERN-OHL-P-2.0](LICENSE-HARDWARE) | Original KiCad/design and manufacturing material in `hardware/`. |
| Documentation | [CC BY 4.0](LICENSE-DOCUMENTATION) | Original README text, written documentation, diagrams, photos, and screenshots. |
| External material | [Third-party notices](THIRD-PARTY-NOTICES.md) | Vendor data, third-party libraries/models, dependencies, logos, and other material not created by JSI Tech. |

The [collection licensing guide](../../LICENSE-SCOPE.md) explains the boundary,
required notices, and limitations.
