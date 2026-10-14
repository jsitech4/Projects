# NFC Card Payment System

## Overview

This project is an NFC Card Payment System built around the **ESP32-S3-DEVKIT** using a modular **PlatformIO** architecture.

The system is designed as a **prototype offline payment terminal** where:

- A user selects a transaction amount using navigation buttons
- An NFC card is tapped on the reader
- The card UID is checked against records stored on the **microSD card**
- The selected amount is deducted from the stored balance
- The transaction is logged with timestamp from the **RTC**
- A local Wi-Fi access point and web page are used to **register cards** and **top up balances**

This project is structured so that the **SD card acts as the backend** for card data and transaction logs, while the OLED provides the local user interface.

The NFC side in this version is a **prototype UID-based system** intended for development, testing, and demonstration.

---

# Features

- ESP32-S3 based payment terminal prototype
- OLED user interface with animated screens
- NFC / RFID UID card detection
- Button-based transaction amount selection
- Local Wi-Fi access point with simple web server
- Web-based card registration and balance top-up
- SD card backend for card database and transaction logs
- RTC timestamped transaction logging
- Non-blocking buzzer feedback patterns
- RGB status LED indication
- Battery voltage monitoring with on-screen battery icon
- Modular PlatformIO project structure
- Designed for easy expansion and maintenance

---

# System Architecture

## Core Operation

The payment terminal works in four main stages:

1. **Amount Selection**  
   The operator uses the buttons to select the amount to charge.

2. **Card Tap**  
   The customer taps an NFC/RFID card on the reader.

3. **Balance Processing**  
   The UID is matched against the SD-stored card database and the selected amount is deducted if sufficient balance exists.

4. **Logging and Feedback**  
   The result is shown on the OLED, buzzer and LED feedback are triggered, and the transaction is written to the SD card with RTC timestamp.

---

## Registration and Top-Up

A simple local web server is hosted by the ESP32-S3 in Access Point mode.  
This is used to:

- register a new card
- update card owner name
- assign opening balance
- top up an existing card balance
- inspect exported card database

This keeps the device self-contained without requiring cloud services.

---

# Hardware Used

## Main Controller
- ESP32-S3-DEVKIT N16R8

## Display
- 2.42-inch 128x64 OLED
- I2C interface
- U8g2 driver:
  `U8G2_SSD1309_128X64_NONAME0_F_HW_I2C`

## Storage
- MicroSD card module

## Timekeeping
- RTC module

## NFC Prototype
- RC522 / prototype UID-based reader on shared SPI bus

## Input
- 5 navigation buttons
- External pull-up button wiring
- Optional external keyboard workflow through ESP32-S3 USB path for future expansion

## Indicators / Alerts
- Onboard RGB LED
- Active buzzer

## Power Monitoring
- Li-ion battery sensing through voltage divider

---

# Pin Configuration

## I2C Bus

| Function | GPIO |
|---|---|
| OLED SDA | GPIO8 |
| OLED SCL | GPIO9 |
| RTC SDA | GPIO8 |
| RTC SCL | GPIO9 |

---

## SPI Bus (Shared)

| Function | GPIO |
|---|---|
| MOSI | GPIO11 |
| SCK | GPIO12 |
| MISO | GPIO13 |

---

## Chip Select / Shared Control Lines

| Function | GPIO |
|---|---|
| CS1 | GPIO10 |
| CS2 | GPIO15 |
| CS3 | GPIO5 |

### Firmware Assumption
- NFC SS / SDA -> **GPIO5**
- SD Card CS -> **GPIO10**
- Buzzer -> **GPIO15**

---

## NFC Prototype

| Function | GPIO |
|---|---|
| NFC SS | GPIO5 |
| NFC RST | GPIO4 |

---

## Buttons

| Function | GPIO |
|---|---|
| Up | GPIO2 |
| Down | GPIO6 |
| Left | GPIO7 |
| Right | GPIO16 |
| OK | GPIO3 |

---

## Other I/O

| Function | GPIO |
|---|---|
| Onboard RGB LED | GPIO48 |
| Battery ADC | GPIO1 |
| Buzzer | GPIO15 |

---

# Battery Voltage Divider

The battery voltage must **not** be connected directly to the ESP32-S3 ADC pin.

Use a voltage divider between the battery and the ADC input.

Example concept:

Battery+ ---- R1 ---- ADC ---- R2 ---- GND

The firmware currently uses a scaling factor for interpreted battery voltage.  
Verify the resistor values on the assembled board and adjust the scaling factor if required.

---

# OLED UI

The OLED handles the local terminal interface and includes:

- boot / welcome screen
- home screen with selected amount
- animated **TAP TO PAY** screen
- payment success / failure message screens
- card registration prompt
- battery icon
- network status icon

The display driver used in firmware is:

```cpp
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
```

---

# Payment Logic

The current firmware uses preset selectable payment amounts such as:

- 100
- 200
- 500
- 1000
- 2000
- 5000
- 10000

A selected amount is deducted from the card balance when a valid registered card is tapped.

If the card:
- is not registered -> registration flow is triggered
- has insufficient balance -> transaction fails
- has enough balance -> transaction succeeds and is logged

---

# SD Card Backend

The SD card is used as the local backend for:

- card database
- pending registrations
- transaction logs
- exported CSV inspection

Typical files:

- `/db/cards.csv`
- `/logs/transactions.csv`

### Example Cards Database

```csv
uid,name,balance
A1B2C3D4,Raymond,5000
B5C6D7E8,Ada,3000
```

### Example Transaction Log

```csv
timestamp,uid,name,amount,balance_after,status
2026-05-08 10:30:15,A1B2C3D4,Raymond,1000,4000,SUCCESS
2026-05-08 10:31:02,B5C6D7E8,Ada,500,2500,SUCCESS
```

---

# Local Web Server

The ESP32-S3 hosts a simple Access Point and local web dashboard for:

- register / update card
- top up balance
- inspect card database
- view selected amount and pending UID state

### Access Point
- SSID: `NFC_PAYMENT_TERMINAL`
- Password: `*****`

For public GitHub documentation, sensitive or deployment-specific values should remain hidden.

---

# Operating Guide

## Power On

1. Power the ESP32-S3 terminal
2. Wait for the OLED boot screen
3. Ensure SD, RTC, NFC, and display initialize correctly
4. Confirm the local AP is active if registration is needed

---

## Register a New Card

1. Tap an unregistered card on the NFC reader
2. Connect a phone or laptop to the device AP
3. Open the local web page from the terminal IP
4. In **Register / Update Card**, leave UID blank to use the pending scanned UID
5. Enter:
   - card holder name
   - opening balance
6. Save the card

---

## Top Up an Existing Card

1. Open the local web page
2. Enter the UID of the card
3. Enter the top-up amount
4. Submit the form
5. The new balance is saved to the SD database

---

## Make a Payment

1. Use the buttons to select the transaction amount
2. Press **OK**
3. Present the NFC card
4. Wait for success or failure indication
5. The result is logged automatically

---

# Status Feedback

## LED Indicator
- Breathing green -> system normal
- Solid blue -> active payment screen / waiting for card
- Solid red -> system fault

## Buzzer Feedback
- short beep -> navigation / button feedback
- success pattern -> payment success
- failure pattern -> insufficient funds or error
- register pattern -> unknown card / registration required

---

# Project Structure

```text
lib/
├── Actuators/
│   └── buzzer/
├── Communication/
│   └── WiFi/
├── Control/
│   ├── error_handling/
│   └── sleep_wake/
├── Display/
│   ├── led_indicator/
│   └── oled_screen/
├── Input/
│   ├── button/
│   └── NFC_module/
├── Pay/
│   ├── payment/
│   └── transaction_log/
├── Pins/
├── Power/
│   └── battery_level/
├── Storage/
│   ├── p_data/
│   └── sd_card/
├── System/
│   ├── reset/
│   └── rtc/

src/
└── main.cpp
```

---

# PlatformIO Environment

```ini
[env:esp32-s3-devkitm-1]
platform = espressif32
board = esp32-s3-devkitm-1
framework = arduino
monitor_speed = 115200
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

# Important Notes

- This project is a **prototype NFC / RFID payment system**
- The card UID is used as an identifier only
- The actual balance is stored on the SD card backend
- Verify battery divider scaling against the real hardware
- Ensure the OLED module is configured for the correct interface mode
- Keep SPI chip-select assignments consistent with your wiring
- The local AP password should be changed for field deployment
- Button GPIO3 is used in this prototype design; review hardware constraints if revising the PCB

---

# Future Improvements

- USB keyboard registration flow on ESP32-S3 host path
- Better admin dashboard
- stronger card authentication
- transaction history page on local web server
- PIN verification flow
- user account search and edit tools
- balance enquiry screen
- receipt export
- OTA update support
- cloud sync option
- audit tools for transaction history

---

# Security Note

Sensitive information such as:
- passwords
- API keys
- private credentials
- deployment-specific secrets

should always be hidden before publishing to GitHub using:

*****

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

The `hardware/` directory carries the editable KiCad project, PCB, schematic,
custom symbols, footprints, and 3D assets. `documentation/` contains system
diagrams and board imagery, while `production/` carries the BOM, Gerbers,
positions, designators, and IPC netlist. These manufacturing outputs describe
the included revision only; modify the KiCad source and regenerate them for a
new build.

`code/` is modular Arduino/PlatformIO firmware for `esp32-s3-devkitm-1` at
115200 baud. It separates NFC input, payment and transaction-log handling,
RTC, SD-card storage, OLED/LED/buzzer interface, button input, Wi-Fi, battery
handling, reset, and error/sleep logic. The project is a payment-system
prototype: it is not a certified payment terminal and does not by itself
provide PCI compliance, cryptographic key management, fraud control, or
regulatory approval.

The asset-type table below covers original firmware, KiCad design files, original project exports,
images, and documentation. NFC/ESP32/RTC vendor material, libraries, payment
standards, external models, and trademarks retain their own licenses and
requirements. See the [license scope guide](../../LICENSE-SCOPE.md) before
commercial, financial, or production use.

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
