# Fingerprint and RFID Card Attendance System

## Overview

This project is a prototype Fingerprint and RFID Card Attendance System built around the ESP32-S3 DevKitC-1 module. The system is designed for schools, offices, workspaces, laboratories, and small organizations that need a simple local attendance device without depending on an online cloud server.

The main idea of this project is two-way attendance verification. A user must first tap a registered RFID card and then verify with the matching registered fingerprint before attendance is saved. This means the card alone cannot take attendance, and the fingerprint alone cannot take attendance. Both must belong to the same registered user.

The ESP32-S3 hosts a simple local web page, while the SD card inside the device acts as the backend storage for user records, attendance logs, and configuration settings.

The project is designed using a modular PlatformIO structure so that each part of the system can be tested, maintained, and upgraded independently.

---

# Features

- ESP32-S3 based attendance system
- RFID card and fingerprint AND verification
- AS608 fingerprint sensor support
- RC522 RFID card reader support
- SSD1309 128×64 OLED display using U8g2
- DS3231 RTC for date and time logging
- SD card backend storage
- Local web server hosted by the ESP32-S3
- Add/register user from the web page
- Remove/delete user from the web page
- Delete user removes the RFID link and fingerprint template
- Attendance IN and OUT mode
- Configurable for school or workspace use
- Download users, attendance, and config files from the web page
- Clear attendance log from the web page
- Simple OLED interface with minimal display lines
- Prototype keyboard/serial command support
- Modular PlatformIO code structure

---

# System Concept

The system works as a local attendance terminal. A user is registered with three main details:

- User ID
- RFID card UID
- Fingerprint template ID

During attendance, the user taps the RFID card first. The system checks the card against the records stored on the SD card. If the card is registered, the system asks for the fingerprint. The fingerprint sensor then searches for the fingerprint template. Attendance is saved only when the fingerprint ID matches the same user linked to the RFID card.

This gives the system stronger identity verification than using only card or fingerprint alone.

---

# Authentication Flow

```text
Tap RFID Card
      ↓
Check RFID UID in users.csv
      ↓
If card is registered, ask for fingerprint
      ↓
Scan fingerprint using AS608 sensor
      ↓
Compare fingerprint ID with the same user record
      ↓
If both match, save attendance to SD card
```

Attendance is not saved if:

- The RFID card is unknown
- The fingerprint is unknown
- The fingerprint belongs to another user
- The SD card is not ready
- The authentication timeout expires

---

# Web Page Functions

The ESP32-S3 creates a local Wi-Fi access point and hosts a simple web page. The web page reads from and writes to the SD card.

## Home Page

The home page shows:

- System name
- Workspace or school name
- Device IP address
- Current attendance mode
- Number of registered users
- Number of attendance records
- SD card status
- Last system event

It also includes quick forms for:

- Setting attendance mode
- Adding/registering a user
- Removing/deleting a user
- Downloading CSV files

## Users Page

The users page displays the content of `users.csv`. It also allows:

- Adding a user
- Removing a user

When a user is removed, the system removes the user from the SD card, unlinks the RFID card, and deletes the fingerprint template from the AS608 sensor.

## Attendance Page

The attendance page displays the content of `attendance.csv`. It also allows the attendance log to be cleared.

Clearing attendance does not delete registered users.

## Config Page

The config page allows the device to be configured for either school or workspace use.

Configurable fields include:

- System name
- Workspace or school name
- System type
- Default attendance mode
- Access Point SSID
- Access Point password

If the AP SSID or password is changed, the device should be restarted after saving.

---

# SD Card Backend

The SD card serves as the local backend storage device for the project.

The main files on the SD card are:

```text
/config.txt
/users.csv
/attendance.csv
```

## config.txt

Stores system configuration.

```text
system_name=Fingerprint RFID Attendance System
workspace_name=Prototype Workspace
workspace_type=school
ap_ssid=AttendanceSystem
ap_password=********
default_mode=IN
```

## users.csv

Stores registered user information.

```text
user_id,name,workspace,rfid_uid,fingerprint_id,active,created_at
```

Example:

```text
STU001,John Doe,Main,A1B2C3D4,1,1,2026-05-07 08:01:22
```

## attendance.csv

Stores attendance records.

```text
timestamp,user_id,name,method,direction,workspace,credential,record_hash
```

Example:

```text
2026-05-07 08:04:10,STU001,John Doe,RFID+FINGER,IN,Main,A1B2C3D4|FP:1,hash_value
```

---

# Hardware Used

## Main Controller

- ESP32-S3-DEVKITC-1-N8R8

## Identification Modules

- AS608 fingerprint sensor
- RC522 RFID card reader

## Storage and Timekeeping

- MicroSD card module
- DS3231 RTC module

## Display and User Interface

- SSD1309 128×64 OLED display
- Push buttons
- Buzzer
- Optional prototype keyboard/serial command input

## Power Section

- 5 V input supply
- LM2596S 3.3 V regulator section
- Common 3.3 V and 5 V rails

---

# Pin Configuration

## I2C Bus

| Function | ESP32-S3 GPIO |
| -------- | ------------- |
| OLED SDA / RTC SDA | GPIO8 |
| OLED SCL / RTC SCL | GPIO9 |

## Shared SPI Bus

| Function | ESP32-S3 GPIO |
| -------- | ------------- |
| SPI MOSI | GPIO11 |
| SPI SCK  | GPIO12 |
| SPI MISO | GPIO13 |

## RFID and SD Card Chip Select

| Function | ESP32-S3 GPIO |
| -------- | ------------- |
| RC522 SS | GPIO10 |
| RC522 RST | GPIO4 |
| SD Card CS | GPIO15 |

## Fingerprint Sensor UART

| Function | ESP32-S3 GPIO |
| -------- | ------------- |
| ESP32 TX to fingerprint RX | GPIO17 |
| ESP32 RX from fingerprint TX | GPIO18 |

## Buttons

| Button | ESP32-S3 GPIO |
| ------ | ------------- |
| BTN1 | GPIO1 |
| BTN2 | GPIO6 |
| BTN3 | GPIO7 |
| BTN4 | GPIO16 |
| BTN5 | GPIO3 |

## Other Pins

| Function | ESP32-S3 GPIO |
| -------- | ------------- |
| Buzzer | GPIO5 |
| Battery ADC | GPIO2 |
| USB D- | GPIO19 |
| USB D+ | GPIO20 |

GPIO19 and GPIO20 are reserved for USB use on the ESP32-S3. In the current firmware, the status LED is disabled in the pin file to avoid interfering with future USB keyboard support.

---

# OLED Display

The OLED uses the SSD1309 128×64 U8g2 constructor:

```cpp
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
```

The display is intentionally kept minimal. It only shows short messages such as:

- System starting
- Tap card first
- Place finger now
- Attendance saved
- Authentication failed
- SD card error
- User registered
- User deleted

---

# Project Structure

```text
lib/
├── Actuators/
│   └── buzzer/
├── Communication/
│   ├── local_server/
│   └── WiFi/
├── Control/
│   └── attendance_manager/
├── Display/
│   ├── led_indicator/
│   └── oled_screen/
├── Input/
│   ├── buttons/
│   ├── fingerprint/
│   ├── keyboard_input/
│   └── RFID/
├── Pins/
├── Power/
│   └── battery_level/
├── Storage/
│   └── sd_card/
└── System/
    ├── error_handling/
    ├── reset/
    ├── rtc/
    └── sleep_wake/

src/
└── main.cpp
```

---

# Module Summary

## Pins

Contains all GPIO assignments from the schematic. This makes it easier to update pins later without editing every module.

## Attendance Manager

Handles the main logic of the project:

- User enrollment
- RFID and fingerprint linking
- AND verification
- Attendance logging
- Delete user coordination
- IN and OUT attendance mode

## Local Server

Hosts the web page and provides routes for:

- Home
- Users
- Attendance
- Config
- Add user
- Delete user
- Save config
- Download CSV files
- Clear attendance log
- Status API

## SD Card

Stores all backend files used by the web page and firmware.

## Fingerprint

Handles AS608 fingerprint enrollment, search, and template deletion.

## RFID

Handles RC522 card scanning and UID reading.

## OLED Screen

Shows short system status messages.

## RTC

Provides timestamp for attendance records.

## Wi-Fi Manager

Creates the ESP32-S3 access point for the local server.

## Keyboard Input

Provides prototype serial commands such as:

```text
IN
OUT
STATUS
ENROLL,STU001,John Doe,Main
DELETE,STU001
```

---

# Operating Guide

## 1. Power On

1. Insert the SD card.
2. Power the device.
3. Wait for the OLED to show that the system is ready.
4. The device creates a Wi-Fi access point.

Default AP details:

```text
SSID: AttendanceSystem
Password: ********
```

The default IP is usually:

```text
http://192.168.4.1
```

## 2. Connect to the Web Page

1. Connect a phone or laptop to the device Wi-Fi.
2. Open a browser.
3. Enter the device IP address.
4. The local attendance web page will open.

## 3. Add / Register User

1. Open the web page.
2. Enter the user ID, name, and workspace/class/department.
3. Click **Start Registration**.
4. Tap the RFID card on the reader.
5. Place the finger when the OLED asks.
6. The system saves the user to the SD card.

The user is now linked to both RFID card and fingerprint.

## 4. Take Attendance

1. Select IN or OUT mode.
2. Tap the registered RFID card.
3. Place the matching registered finger.
4. If both match the same user, attendance is saved.

## 5. Remove User

1. Open the Users page or Home page.
2. Enter the user ID.
3. Click **Delete User**.
4. The system removes the user record from the SD card.
5. The RFID card is automatically unlinked.
6. The fingerprint template is deleted from the AS608 sensor.

## 6. Download Records

Use the download links to export:

- Users CSV
- Attendance CSV
- Config file

## 7. Clear Attendance

The attendance log can be cleared from the Attendance page.

This only clears `attendance.csv`. It does not delete users.

---

# PlatformIO Environment

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32@6.5.0
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
lib_ldf_mode = deep+

lib_deps =
    olikraus/U8g2@^2.35.30
    adafruit/Adafruit Fingerprint Sensor Library@^2.1.3
    miguelbalboa/MFRC522@^1.4.11
    adafruit/RTClib@^2.1.4
    adafruit/Adafruit BusIO@^1.16.1

build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
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

# Security and Privacy Notes

This is still a prototype, but the following privacy ideas are considered:

- The system does not store raw fingerprint images on the SD card.
- Fingerprint templates are stored inside the AS608 fingerprint sensor.
- The SD card stores user information and attendance records in CSV format.
- Attendance records include a SHA-256 hash for basic tamper checking.
- The local web page is hosted on the ESP32-S3 access point and is intended for local use.
- Sensitive information such as passwords should be hidden before publishing to GitHub.

For a production version, stronger security can be added, such as encrypted SD card records, login authentication for the web page, stronger access control, and automatic backup.

---

# Important Notes

- This project is a prototype.
- The SD card must be inserted before startup.
- The fingerprint sensor storage size depends on the exact AS608 module used.
- The current firmware limits fingerprint IDs to 1–200.
- The attendance method is strictly RFID plus fingerprint.
- Do not use GPIO19 and GPIO20 for normal LEDs if USB support is needed.
- Keep all grounds common between the ESP32-S3 and external modules.
- Use stable power because the SD card and fingerprint sensor can draw current during operation.

---

# Future Improvements

- Full USB HID keyboard support
- Admin login for the local web page
- Better user table display instead of raw CSV preview
- Encrypted SD card records
- Backup and restore function
- Export by date range
- OTA firmware update
- Real-time dashboard refresh
- Better attendance analytics
- Cloud sync option

---

## Security and local configuration

Never commit populated `.env` files, Wi-Fi credentials, passwords, API keys, SMTP credentials, broker tokens, private certificates, or production connection strings. Keep them in an ignored local configuration file or approved secret store, and commit only a redacted `.env.example` when configuration is needed. Replace development access-point credentials before deployment and limit local administration to trusted networks.

# Author

Egbe Raymond

Electrical / Electronic Engineer  
Embedded Systems Developer  
Full Stack Developer

---

## License

Released under the [MIT License](LICENSE).
