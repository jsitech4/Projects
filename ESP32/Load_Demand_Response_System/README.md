# Load Demand Response System

## Overview

This project is a modular Load Demand Response System built around the ESP32-WROOM-32 using a structured PlatformIO architecture.

The system monitors the real electrical load using a PZEM-004T v3.0 energy meter and automatically manages household loads between the inverter supply and PHCN/Grid supply based on:

- Actual measured load power
- User-configured inverter power rating
- User-configured load margin percentage
- User-configured power value for each load
- PHCN/Grid availability
- Inverter availability

The system is designed to reduce inverter overload by intelligently selecting which loads should remain on inverter and which loads should be moved to PHCN/Grid when needed.

The device includes:

- ST7920 128x64 graphical LCD display
- Rotary encoder setup menu
- PCA9555 I/O expander for relay and source sensing
- PZEM-004T v3.0 energy monitoring
- Automatic source management
- Local Wi-Fi web dashboard
- Breathing heartbeat LED
- Non-blocking firmware design

---

# Features

- ESP32-WROOM-32 based controller
- PZEM-004T v3.0 live energy monitoring
- Voltage, current, power, and energy display
- Six-load energy management
- Inverter and PHCN/Grid source decision logic
- PCA9555 GPIO expander support
- ST7920 128x64 LCD user interface
- Rotary encoder menu navigation
- First-boot setup mode
- Runtime setup access using long encoder press
- Local web dashboard through ESP32 Access Point mode
- Configurable relay load power values
- Configurable inverter power rating
- Configurable load margin percentage
- Preferences-based setting storage
- Non-blocking firmware architecture
- Modular PlatformIO project structure

---

# Hardware Used

## Main Controller

- ESP32-WROOM-32 development board

## Display

- ST7920 128x64 graphical LCD

## Energy Monitoring

- PZEM-004T v3.0 AC energy monitoring module

## GPIO Expansion

- PCA9555 16-bit I/O expander

## User Input

- Rotary encoder with push button

## Output Devices

- Six relay-controlled load channels
- Source switching relay channels
- Heartbeat LED
- Indicator output from PCA9555

## Power Source Monitoring

- PHCN/Grid sense input
- Inverter sense input

---

# Pin Configuration

## ESP32-WROOM-32 Pin Assignment

| Function | GPIO |
|---|---|
| Heartbeat LED | GPIO2 |
| ST7920 LCD CS | GPIO5 |
| ST7920 LCD SCLK | GPIO18 |
| ST7920 LCD MOSI | GPIO23 |
| PCA9555 SDA | GPIO21 |
| PCA9555 SCL | GPIO22 |
| PZEM RX to ESP32 | GPIO16 |
| PZEM TX from ESP32 | GPIO17 |
| Encoder A / CLK | GPIO26 |
| Encoder B / DT | GPIO25 |
| Encoder SW | GPIO27 |

---

# ST7920 LCD Connection

| LCD Function | ESP32 GPIO |
|---|---|
| CS / RS | GPIO5 |
| SCLK / E | GPIO18 |
| MOSI / RW | GPIO23 |

The ST7920 must be placed in serial/SPI mode.

---

# PZEM-004T v3.0 Connection

| PZEM Function | ESP32 GPIO |
|---|---|
| PZEM TX | GPIO16 |
| PZEM RX | GPIO17 |

The PZEM is used as the actual power sensor for the system. The firmware uses the measured PZEM power value for energy-management decisions.

---

# PCA9555 Configuration

Default PCA9555 I2C address:

```text
0x20
```

## PCA9555 Pin Assignment

| PCA9555 Pin | Function |
|---|---|
| P00 | Load 1 source relay |
| P01 | Load 1 load relay |
| P02 | Load 2 source relay |
| P03 | Load 2 load relay |
| P04 | Load 3 source relay |
| P05 | Load 3 load relay |
| P06 | Load 4 source relay |
| P07 | Load 4 load relay |
| P10 | Load 5 source relay |
| P11 | Load 5 load relay |
| P12 | Load 6 source relay |
| P13 | Load 6 load relay |
| P14 | Indicator output |
| P15 | PHCN/Grid sense input |
| P16 | Inverter sense input |
| P17 | 12V enable output |

---

# Source Sense Logic

The firmware reads PHCN/Grid and inverter availability from the PCA9555 input pins.

The active logic is controlled in `Pins.cpp`:

```cpp
const bool SOURCE_SENSE_ACTIVE_HIGH = true;
const bool INVERTER_SENSE_ACTIVE_HIGH = true;
```

If the source-sensing circuit gives LOW when the source is available, change the values to:

```cpp
const bool SOURCE_SENSE_ACTIVE_HIGH = false;
const bool INVERTER_SENSE_ACTIVE_HIGH = false;
```

---

# Load Management Logic

The system does not depend on fixed assumed load values for its running decision.

It uses:

- Actual live power from the PZEM sensor
- User-saved inverter power rating
- User-saved load margin percentage
- User-saved load power value for each relay channel
- Real PHCN/Grid availability input
- Real inverter availability input

## Effective Inverter Limit

The effective inverter limit is calculated using:

```text
Effective Limit = Inverter Power Rating x Load Margin Percentage
```

Example:

```text
Inverter Rating = 5000 W
Load Margin = 90 %
Effective Limit = 4500 W
```

## Relay Source Meaning

```text
INVERTER = Load remains powered from inverter
PHCN     = Load is moved to grid/PHCN
```

When the load is within a safe range, the system keeps loads on inverter.

When the measured load approaches or exceeds the safe inverter limit and PHCN/Grid is available, the system selects which loads should remain on inverter and moves other loads to PHCN/Grid to protect the inverter.

If the inverter is unavailable, the system moves the loads away from inverter.

---

# LCD Operation Guide

The LCD shows two main runtime screens:

## Load Status Screen

This screen shows the source status of each load channel.

```text
INV = Load is on inverter
GRD = Load is on PHCN/Grid
```

## Consumption Screen

This screen shows live PZEM readings:

- Voltage
- Current
- Power
- Energy

The device alternates between the load status screen and the consumption screen during normal operation.

---

# Encoder Operation Guide

## First Boot Setup

If the device has not been configured before, it automatically enters setup mode.

## Boot Setup Entry

During startup, hold the encoder button within the boot window to enter setup mode.

```text
Boot window: 5 seconds
Required hold time: 3 seconds
```

## Runtime Setup Entry

During normal operation, hold the encoder button to enter setup mode.

```text
Required hold time: 5 seconds
```

## Menu Navigation

| Action | Function |
|---|---|
| Rotate encoder | Move through menu / change value |
| Press encoder | Select item / move to next setting |
| Long press encoder | Enter setup mode from runtime |

---

# Setup Menu

The setup menu contains:

- Relay Mode
- Timer Setup
- WiFi Config
- System Setup
- Exit

## Relay Mode

Relay Mode is used to set the configured power rating for each load channel.

The six load channels are configured one after another:

```text
Relay 1 Load Power
Relay 2 Load Power
Relay 3 Load Power
Relay 4 Load Power
Relay 5 Load Power
Relay 6 Load Power
```

The configured load values are saved to ESP32 Preferences memory.

## System Setup

System Setup is used to set:

- Inverter Power Rating
- Load Margin Percentage

The inverter power rating should match the real inverter capacity.

The load margin percentage defines how much of the inverter rating should be used before the system starts protecting the inverter.

## WiFi Config

WiFi Config displays the local server information.

Default Access Point:

```text
SSID: SHEMS-Controller
Password: *****
IP Address: 192.168.4.1
```

The password is masked in this README for GitHub publishing. The actual value can be changed in the local server module.

---

# Local Web Dashboard

The ESP32 creates a local Wi-Fi Access Point.

Connect a phone or laptop to the device Wi-Fi network, then open:

```text
http://192.168.4.1
```

The dashboard displays:

- Voltage
- Current
- Actual load power
- Energy
- PHCN/Grid status
- Inverter status
- Effective inverter limit
- Load-management decision
- Relay source status
- Relay configured load power values

The dashboard also allows updating the inverter rating, load margin, and relay load power values.

---

# Default Configuration

| Parameter | Default Value |
|---|---:|
| Relay 1 Load Power | 500 W |
| Relay 2 Load Power | 700 W |
| Relay 3 Load Power | 900 W |
| Relay 4 Load Power | 1200 W |
| Relay 5 Load Power | 1500 W |
| Relay 6 Load Power | 2000 W |
| Inverter Power Rating | 5000 W |
| Load Margin | 90 % |

These values can be changed from the LCD setup menu or the local web dashboard.

---

# Project Structure

```text
lib/
├── Actuators/
│   ├── load_relay/
│   └── source_relay/
├── Control/
│   ├── error_handling/
│   └── sleep_wake/
├── Display/
│   ├── lcd_screen/
│   └── led_indicator/
├── Drivers/
│   └── gpio_expander/
├── Input/
│   └── rotary_encoder/
├── Pins/
├── Power/
│   ├── inverter_sense/
│   ├── nepa_sense/
│   └── regulator/
├── Sensors/
│   └── pzem_sensor/
├── System/
│   ├── config_manager/
│   ├── load_manager/
│   ├── local_server/
│   ├── ndelay/
│   ├── reset/
│   └── shared_var/

src/
└── main.cpp
```

---

# PlatformIO Environment

```ini
[platformio]
default_envs = esp32dev

[env:esp32dev]
platform = espressif32@6.5.0
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    olikraus/U8g2@^2.36.17
    mandulaj/PZEM-004T-v30@^1.2.1
build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=0
```

---

# Build and Upload

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

# Operation Flow

1. Power on the system.
2. The ESP32 initializes the LCD, encoder, PCA9555, relays, PZEM, local server, and saved configuration.
3. If the system has not been configured before, setup mode opens automatically.
4. Configure relay load power values and inverter settings.
5. During runtime, the PZEM measures actual voltage, current, power, and energy.
6. The source-sense inputs report PHCN/Grid and inverter availability.
7. The load manager checks the actual load against the inverter safe limit.
8. If the load is safe, loads remain on inverter.
9. If the inverter is close to overload and PHCN/Grid is available, selected loads are moved to PHCN/Grid.
10. The LCD and web dashboard show the current system state.

---

# Important Notes

- The PZEM must be wired correctly because the system uses it for live load decisions.
- Do not use assumed sensor values during final deployment.
- Configure the actual inverter rating before testing the load-management behavior.
- Configure each relay load power according to the real appliance connected to that load channel.
- Confirm the PHCN/Grid and inverter sense logic before connecting real loads.
- Use properly rated relays, contactors, and wiring for AC load switching.
- Use isolation and proper protection when working with mains voltage.
- The firmware is designed to be non-blocking and should not use `delay()` in the main control flow.

---

# Safety Warning

This project involves AC mains power and inverter/grid source switching. All high-voltage wiring should be handled carefully and tested with proper isolation, fusing, earthing, and protection devices.

For real installation, use correctly rated relays or contactors and ensure there is no unsafe back-feed between inverter and PHCN/Grid sources.

---

# Future Improvements

- Real-time graphing on the local dashboard
- Manual override from the web dashboard
- Load priority setup from LCD
- OTA update support
- Cloud logging
- MQTT integration
- Mobile app integration
- SD card logging
- Historical energy report

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

The editable design record in `hardware/` includes the primary KiCad project,
PCB and schematic plus separate block-diagram and schematic-only sources.
`documentation/` provides diagram PDFs and board images; `production/`
provides the BOM, Gerbers, positions, designators, and IPC netlist. Use the
KiCad files to inspect clearances, net names, ratings, and relay wiring before
making or changing a board; fabrication files are release outputs.

The `code/` PlatformIO/Arduino firmware builds for `esp32dev` at 115200 baud.
Its architecture separates PZEM metering, source and load relays, regulator
and GPIO-expander drivers, LCD/LED status, rotary-encoder input, Wi-Fi/local
interface, and control/error/sleep logic. Energy readings and switching
decisions should be calibrated and validated against an approved meter and
isolated test loads before connection to a live installation.

Original design sources, firmware, original exported manufacturing artifacts,
images, and documentation are licensed by asset type below. Meter, relay, ESP32, and
other vendor documents/components; third-party libraries; and their trademarks
keep their own terms. The [license scope guide](../../LICENSE-SCOPE.md) states
the reuse boundary and does not substitute for electrical or regulatory review.

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
