# Solar Powered Smart Irrigation System

## Overview

This project is a Solar Powered Smart Irrigation System built around the ESP32 microcontroller. The system is designed to automatically manage water pumping for an overhead tank and irrigation sprinkler without requiring constant manual control.

The system uses an ultrasonic sensor to monitor the water level inside the overhead tank, a soil moisture sensor to determine when irrigation is needed, battery and solar voltage sensing for power monitoring, and a rotary encoder for setup and calibration.

The project is structured using a modular PlatformIO architecture so that each section of the system can be tested, maintained, and upgraded independently.

The system includes:

- Overhead tank water level monitoring
- Automatic tank filling control
- Automatic irrigation sprinkler control
- Dry-run protection
- Overflow protection
- Low water level buzzer alert
- Battery level monitoring and calibration
- Solar input monitoring
- Rotary encoder based setup and calibration
- Saved setup values using ESP32 Preferences memory
- 20x4 I2C LCD user interface
- Non-blocking autonomous operation

---

# Features

- Autonomous irrigation control
- Automatic overhead tank pump control
- Automatic sprinkler pump control
- Ultrasonic water level measurement
- Soil moisture based irrigation decision
- Buzzer warning for low water level and system faults
- Dry-run protection for the irrigation pump
- Overflow protection for the overhead tank
- Pump timeout protection
- Battery voltage calibration
- Solar voltage monitoring
- Rotary encoder setup menu
- Long press setup entry
- Saved calibration values
- Modular PlatformIO code structure
- Non-blocking program flow

---

# Hardware Used

## Main Controller

- ESP32 development board

## Display

- 20x4 I2C LCD screen

## Sensors

- Ultrasonic sensor for overhead tank level monitoring
- Soil moisture sensor for irrigation control
- Battery voltage sensing circuit
- Solar voltage sensing circuit
- DHT21 / AM2301 temperature and humidity sensor

## Inputs

- Rotary encoder with push button

## Actuators

- Pump relay for overhead tank filling
- Pump relay for irrigation sprinkler
- Buzzer for alerts
- Status LED

## Pumps

- 1 × Overhead tank pump
- 1 × Irrigation sprinkler pump

---

# Pin Configuration

## Rotary Encoder

| Signal | GPIO   |
| ------ | ------ |
| CLK    | GPIO25 |
| DT     | GPIO26 |
| SW     | GPIO27 |

## Ultrasonic Sensor

| Signal | GPIO   |
| ------ | ------ |
| TRIG   | GPIO14 |
| ECHO   | GPIO12 |

## Pumps and Actuators

| Device                 | GPIO   |
| ---------------------- | ------ |
| Overhead Tank Pump     | GPIO33 |
| Irrigation Pump        | GPIO16 |
| Buzzer                 | GPIO4  |
| Status LED             | GPIO2  |

## Analog Sensors

| Sensor        | GPIO   |
| ------------- | ------ |
| Soil Sensor   | GPIO34 |
| Battery Sense | GPIO35 |
| Solar Sense   | GPIO39 |

## Temperature and Humidity Sensor

| Signal | GPIO   |
| ------ | ------ |
| DATA   | GPIO32 |

## LCD Display

| Signal | GPIO   |
| ------ | ------ |
| SDA    | GPIO21 |
| SCL    | GPIO22 |

---

# Project Structure

```text
lib/
├── Actuators/
│   ├── buzzer/
│   └── pump/
├── Control/
│   ├── error_handling/
│   └── sleep_wake/
├── Display/
│   ├── lcd_screen/
│   └── led_indicator/
├── Input/
│   └── rotary_encoder/
├── Pins/
├── Power/
│   ├── battery_level/
│   └── solar_level/
├── Sensors/
│   ├── soil_sensor/
│   ├── temp_hum/
│   └── ultrasonic/
├── System/
│   ├── reset/
│   ├── settings_manager/
│   ├── setup_manager/
│   └── system_logic/

src/
└── main.cpp
```

---

# System Operation

## Power On

1. Power the ESP32 and all connected modules.
2. The LCD displays the system status.
3. Saved calibration values are loaded from ESP32 Preferences memory.
4. The system enters autonomous mode automatically.
5. Both pumps remain controlled by the system logic.

---

# Autonomous Control Logic

## Overhead Tank Pump

The overhead tank pump is responsible for filling the overhead tank.

The tank pump turns ON when:

- The overhead tank level is below the saved low tank threshold.
- The ultrasonic sensor reading is valid.
- The system is not in setup mode.

The tank pump turns OFF when:

- The overhead tank reaches the saved full tank threshold.
- The maximum tank pump runtime is exceeded.
- The ultrasonic sensor fails.
- The system enters setup mode.

This protects the system from overflow and pump damage.

---

## Irrigation Sprinkler Pump

The irrigation pump is responsible for watering the farm or garden through the sprinkler system.

The irrigation pump turns ON when:

- The soil moisture level is below the saved dry threshold.
- The overhead tank has enough water.
- The system is not in setup mode.
- The irrigation pause time has completed.

The irrigation pump turns OFF when:

- The soil is wet enough.
- The irrigation run time is completed.
- The overhead tank becomes low.
- The ultrasonic sensor reports an invalid reading.
- The system enters setup mode.

This prevents dry-run operation and avoids wasting water.

---

# Tank Level Monitoring

The ultrasonic sensor measures the distance from the top of the tank to the water surface.

During calibration:

- The full tank distance is saved.
- The empty or low tank reference distance is saved.

The system uses these saved values to calculate the tank level percentage.

Example behavior:

- Smaller distance means the water is closer to the sensor, so the tank is fuller.
- Larger distance means the water is farther from the sensor, so the tank is lower.

---

# Buzzer Alert

The buzzer alerts the user when the system detects an important condition.

The buzzer can activate when:

- The overhead tank level is low.
- The ultrasonic sensor reading is invalid.
- The tank pump runtime exceeds the safety limit.
- The battery level is low.

---

# Rotary Encoder Operation

The rotary encoder is used for setup, calibration, and menu control.

## Entering Setup Mode

Press and hold the encoder button for about 5 seconds.

When setup mode is entered:

- Both pumps are turned OFF for safety.
- The LCD shows the setup menu.
- The system pauses autonomous pump control.

## Menu Navigation

- Rotate encoder clockwise to move forward.
- Rotate encoder anticlockwise to move backward.
- Short press to select or edit an item.
- Rotate while editing to increase or decrease the selected value.
- Press again to save the selected value.
- Long press to save and exit setup mode.

---

# Setup Menu

The setup menu allows the system to be calibrated directly from the device.

## Available Setup Options

1. Calibrate Full Tank
2. Calibrate Empty / Low Tank
3. Set Low Tank Level
4. Set Full Stop Level
5. Set Battery Empty Voltage
6. Set Battery Full Voltage
7. Set Battery Scale Factor
8. Set Soil Dry Threshold
9. Set Irrigation Run Time
10. Set Irrigation Pause Time
11. Set Tank Pump Max Runtime
12. Save and Exit
13. Reset Defaults

---

# Calibration Guide

## Full Tank Calibration

1. Fill the overhead tank to the desired full level.
2. Enter setup mode.
3. Select `Calibrate Full Tank`.
4. Save the current ultrasonic distance.
5. The system will use this value as the full tank reference.

---

## Empty / Low Tank Calibration

1. Allow the tank to reach the desired low or empty level.
2. Enter setup mode.
3. Select `Calibrate Empty / Low Tank`.
4. Save the current ultrasonic distance.
5. The system will use this value as the low tank reference.

---

## Low Tank Threshold

This value determines when the system should treat the tank as low.

When the tank level falls below this threshold:

- The irrigation pump is stopped.
- The tank pump is allowed to start.
- The buzzer can alert the user.

---

## Full Stop Threshold

This value determines when the tank pump should stop filling the overhead tank.

When the tank reaches this threshold:

- The tank pump turns OFF.
- Overflow is prevented.

---

## Battery Calibration

Battery calibration allows the system to show a more accurate battery percentage.

The setup menu includes:

- Battery empty voltage
- Battery full voltage
- Battery scale factor

The scale factor should match the voltage divider used on the battery sensing circuit.

---

## Soil Moisture Calibration

The soil dry threshold determines when the irrigation pump should turn ON.

A lower or higher value may be required depending on the soil sensor type, soil condition, and installation depth.

---

# Saved Settings

All setup values are saved in ESP32 Preferences memory.

This means:

- Calibration is not lost after power off.
- The system starts with the last saved values.
- The device does not need to be calibrated every time it boots.
- Values only change when setup mode is used again.

---

# Safety Behavior

## Dry-Run Protection

The irrigation pump is stopped when the overhead tank is low.

This prevents the sprinkler pump from running without water.

---

## Overflow Protection

The tank pump is stopped when the overhead tank reaches the full stop threshold.

This prevents water overflow.

---

## Setup Mode Protection

Both pumps are forced OFF when setup mode is active.

This makes calibration safer.

---

## Sensor Fault Protection

If the ultrasonic sensor gives an invalid reading:

- Pump operation is stopped.
- The buzzer alerts the user.
- The LCD can show the sensor fault condition.

---

# Operating Guide

## Normal Operation

1. Power the system.
2. Allow the LCD to show the startup screen.
3. The system loads saved settings.
4. The system monitors the tank, soil, battery, and solar input.
5. The tank pump fills the overhead tank when needed.
6. The sprinkler pump irrigates only when the soil is dry and enough water is available.

---

## First-Time Setup

1. Install all sensors and pumps correctly.
2. Fill the overhead tank to the full level and calibrate full tank.
3. Allow the tank to reach the desired low level and calibrate low tank.
4. Set the low tank percentage.
5. Set the full stop percentage.
6. Calibrate the battery empty and full voltages.
7. Set the soil dry threshold.
8. Save and exit setup mode.
9. Restart the system and confirm that saved values are retained.

---

## Daily Use

The system is designed to run by itself.

The user only needs to check:

- Water source availability
- Battery condition
- Solar charging condition
- Pump operation
- Sensor wiring
- LCD status messages

---

# Important Notes

- Do not power pumps directly from the ESP32.
- Use relay modules or proper pump drivers.
- Use an external power supply suitable for the pumps.
- Connect all grounds together when required.
- Use a voltage divider before connecting battery or solar voltage to ESP32 ADC pins.
- GPIO34, GPIO35, and GPIO39 are input-only pins.
- Protect the ultrasonic sensor from direct water splash.
- Keep relay wiring separated from low-voltage signal wiring where possible.
- Use proper fuses and protection for pump power lines.

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

# Uploading the Code

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

# Troubleshooting

## LCD Not Displaying

- Check SDA and SCL wiring.
- Confirm the LCD I2C address.
- Check 5V and GND connection.

## Ultrasonic Reading Is Wrong

- Check TRIG and ECHO wiring.
- Make sure the sensor is facing the water surface.
- Keep the sensor away from splashing water.
- Recalibrate full and low tank levels.

## Pump Does Not Turn ON

- Check relay power supply.
- Check relay signal pin.
- Confirm the system is not in setup mode.
- Check tank level and soil moisture values.
- Confirm saved thresholds are correct.

## Irrigation Pump Stops Immediately

- The tank may be below the low water threshold.
- The soil may already be wet.
- The ultrasonic sensor may be giving an invalid reading.
- The irrigation run time may be too short.

## Settings Are Not Changing

- Make sure the value is saved before exiting setup mode.
- Restart the device and check if the saved value is retained.
- Use reset defaults if the setup values become incorrect.

---

# Security Note

Sensitive information such as:

- WiFi password
- Access point password
- API keys

should be hidden before publishing the project to GitHub.

Use placeholders such as:

```text
*****
```

before uploading the project publicly.

---

# Future Improvements

- Web dashboard for remote monitoring
- WiFi or Blynk integration
- SD card data logging
- RTC based scheduled irrigation
- Solar charge controller integration
- Flow sensor for pump feedback
- Water leakage detection
- Mobile notification support
- Manual override switch
- Enclosure and PCB design

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

The editable electrical source lives in `hardware/` as the KiCad project,
schematic, and PCB, with related library/3D files. `documentation/` contains
system diagrams, ESP32 reference, board/layout images, and photographs;
`production/` supplies Gerbers, BOM, component positions, designators, and IPC
netlist. Rework the KiCad source rather than modifying export files, then
generate a matched manufacturing release.

`code/` is Arduino/PlatformIO firmware for `esp32dev` at 115200 baud. Its
structure separates soil, temperature/humidity, and ultrasonic sensing; pump,
buzzer, LED, LCD, rotary input, battery/solar monitoring; settings/setup and
irrigation logic; reset/error/sleep handling. Field commissioning requires
sensor calibration for the actual soil/tank, pump flow and dry-run checks,
correct fuse/wire sizing, suitable weatherproofing, and independent safeguards
against flooding or battery damage.

The asset-type table below covers original firmware, KiCad design material, project-generated BOM and
fabrication outputs, images, and documents. Vendor datasheets, libraries,
models, parts, and branding retain their own terms. The [license scope guide](../../LICENSE-SCOPE.md)
explains reuse limits and the lack of safety or performance warranties.

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
