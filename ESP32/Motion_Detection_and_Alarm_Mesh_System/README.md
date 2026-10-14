# Motion Detection and Alarm Mesh System

## Overview

This project is a Motion Detection and Alarm Mesh System built using four ESP32-WROOM-32 development boards communicating through ESP-NOW wireless communication.

The system is designed as a distributed wireless intruder alarm network where:

- One ESP32 acts as the Master Node
- Three ESP32 boards act as Slave Nodes

The master node monitors motion using a PIR sensor and wirelessly broadcasts alarm triggers to all slave nodes. Each slave node independently activates its local alarm system whenever a trigger is received.

The system also includes:
- Battery voltage monitoring
- Battery level LED bar graph
- Non-blocking alarm logic
- Retriggerable alarm timing
- Local test trigger button
- Modular PlatformIO architecture
- Wireless ESP-NOW communication
- Multi-node distributed alarm response

The project is designed using a modular software structure for easier scalability, debugging, maintenance, and future upgrades.

---

# Features

- Wireless ESP-NOW mesh-style communication
- Master-to-multiple-slave alarm triggering
- PIR motion detection
- Local buzzer alarm on all nodes
- Retriggerable 20-second alarm timer
- Battery voltage monitoring
- 4-level battery LED indicator
- Push-button trigger simulation for testing
- Non-blocking millis()-based logic
- Modular PlatformIO structure
- Expandable node architecture
- Low-latency wireless communication

---

# System Architecture

## Master Node

The master node contains:
- PIR motion sensor
- Buzzer
- Battery monitor
- 4 battery level LEDs
- Test trigger button
- ESP-NOW transmitter

Responsibilities:
- Detect motion
- Send alarm triggers to all slaves
- Sound local alarm
- Monitor battery condition

---

## Slave Nodes

Each slave node contains:
- Buzzer
- Battery monitor
- 4 battery level LEDs
- Test trigger button
- ESP-NOW receiver

Responsibilities:
- Receive alarm trigger from master
- Activate local alarm
- Monitor battery condition

---

# Hardware Used

## Main Controller
- 4 × ESP32-WROOM-32

## Sensors
- PIR Motion Sensor

## Indicators
- 4 × Battery level LEDs
- Status LED

## Inputs
- Push button

## Alarm
- Active buzzer

## Communication
- ESP-NOW wireless communication

---

# Pin Configuration

## Common Pins (All Nodes)

| Function | GPIO |
|---|---|
| Buzzer | GPIO23 |
| Button | GPIO27 |
| Battery ADC | GPIO34 |
| Battery LED1 | GPIO16 |
| Battery LED2 | GPIO17 |
| Battery LED3 | GPIO18 |
| Battery LED4 | GPIO19 |
| Status LED | GPIO2 |

---

## Master Only

| Function | GPIO |
|---|---|
| PIR Sensor | GPIO26 |

---

# Battery Voltage Divider

The battery voltage must NOT be connected directly to the ESP32 ADC pin.

Use a voltage divider:

Battery+ ---- R1 ---- ADC(GPIO34) ---- R2 ---- GND

Recommended:
- R1 = 100k
- R2 = 100k

The software uses a calibration scaling factor for voltage correction.

---

# Alarm Logic

## Alarm Trigger

The alarm can be triggered by:
- PIR motion detection
- Test button press
- Wireless trigger reception

---

## Retrigger Logic

When motion is detected:
- Alarm starts immediately
- 20-second timer begins

If another trigger is received before timeout:
- The timer resets

If no trigger is received for 20 seconds:
- Alarm automatically stops

This creates proper intruder-alarm behavior.

---

# Alarm Pattern

The buzzer uses a repeating alarm pattern:

- ON for 150 ms
- OFF for 100 ms
- ON for 150 ms
- OFF for 400 ms

Pattern repeats continuously during active alarm state.

---

# Battery Level Indicator

## LED Meaning

| LEDs | Battery Status |
|---|---|
| LED1 | Very Low |
| LED1 + LED2 | Low |
| LED1 + LED2 + LED3 | Good |
| LED1 + LED2 + LED3 + LED4 | Full |

---

# ESP-NOW Communication

The system uses ESP-NOW for:
- Low-latency communication
- Router-free operation
- Low power usage
- Direct peer-to-peer communication

The master stores the MAC addresses of all slave nodes and sends alarm packets directly to each slave.

---

# Project Structure

```text
lib/
├── Communication/
│   └── mesh_comm/
├── Sensors/
│   └── pir_sensor/
├── Input/
│   └── button/
├── Actuators/
│   └── buzzer/
├── Power/
│   └── battery_level/
├── Display/
│   └── led_indicator/
├── Pins/

src/
└── main.cpp
```

---

# Operating Guide

## Power On

1. Power all ESP32 nodes
2. Wait for ESP-NOW initialization
3. Verify battery LEDs
4. Verify slave nodes connect properly

---

## Motion Detection

When motion is detected on the master:
- Master buzzer activates
- Alarm packet is sent wirelessly
- All slave buzzers activate
- Alarm continues until timeout

---

## Test Trigger

Press the local button:
- Simulates PIR motion trigger
- Useful during development and testing
- Retriggers the alarm timer

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

# Important Notes

- Use a separate power source for buzzers if required
- All grounds must be connected together
- GPIO34 is input-only
- Avoid using ADC2 pins with WiFi/ESP-NOW
- Ensure all slave MAC addresses are correctly configured
- ESP-NOW works best when nodes share the same WiFi channel

---

# Future Improvements

- True multi-hop mesh forwarding
- Online/offline node monitoring
- ACK feedback system
- OLED status display
- OTA firmware updates
- Battery charging system
- Solar charging integration
- GSM notification system
- Cloud logging
- Mobile application

---

# Security Note

Sensitive information such as:
- WiFi passwords
- API keys
- Private credentials

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

## License

Released under the [MIT License](LICENSE).
