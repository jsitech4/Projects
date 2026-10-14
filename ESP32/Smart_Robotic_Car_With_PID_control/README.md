# Smart Robotic Car With PID Control

## Overview

This project is a Smart Robotic Car built around the ESP32-WROOM-32 microcontroller. I designed the system to move autonomously, detect obstacles, reduce speed gradually when approaching an object, scan the surrounding area with a servo-mounted ultrasonic sensor, and decide the best turning direction using a controlled driving algorithm.

The car uses a Yahboom 4-Channel Motor Drive Module to control four L-shape geared 520 DC motors. The ESP32 communicates with the motor driver through UART. A gyroscope sensor is used for motion feedback, yaw estimation, turning stability, and PID-assisted control. The ultrasonic sensor is mounted on a servo motor so that the car can scan left, center, and right before making a turn.

The project also includes battery monitoring, four battery level indicator LEDs, a breathing power indicator LED, and a local web dashboard for viewing important system information such as distance, speed, battery status, turning mode, servo angle, and motion data.

The firmware is structured using a modular PlatformIO architecture so that each section of the system can be tested, maintained, and upgraded independently.

The system includes:

- ESP32-WROOM-32 main controller
- Yahboom 4-Channel Motor Drive Module
- Four L-shape geared 520 DC motors
- Ultrasonic obstacle detection
- Servo-mounted ultrasonic scanning neck
- MPU6050 gyroscope / accelerometer feedback
- PID-based speed and turn control
- Gradual speed reduction near obstacles
- Autonomous obstacle avoidance
- Manual control through web dashboard
- Battery voltage monitoring
- Four battery level LEDs
- Breathing power indicator LED
- Non-blocking program flow
- Local ESP32 web dashboard

---

# Features

- Autonomous self-driving behavior
- Gradual speed reduction when approaching obstacles
- Servo-based left, center, and right ultrasonic scanning
- Right-turn preference after obstacle scan when the right side is clear
- PID-assisted distance control
- PID-assisted yaw turning control
- Yahboom UART motor driver control
- Four-motor tank-style drive control
- Manual forward, backward, left, right, and stop control from web page
- Live web dashboard for car status
- Battery voltage and percentage monitoring
- Four battery level indicator LEDs
- GPIO2 breathing power indicator LED
- MPU6050 pitch, roll, yaw, vibration, and motion monitoring
- Non-blocking ultrasonic reading
- Non-blocking servo movement
- Non-blocking motor command update
- Modular PlatformIO project structure

---

# Hardware Used

## Main Controller

- ESP32-WROOM-32 development board

## Motor Driver

- Yahboom 4-Channel Motor Drive Module

## Motors

- 4 × L-shape geared 520 DC motors

## Sensors

- Ultrasonic sensor for obstacle detection
- MPU6050 gyroscope / accelerometer sensor
- Battery voltage sensing circuit

## Actuators

- Servo motor for ultrasonic sensor scanning neck
- Four DC motors through Yahboom driver

## Indicators

- 1 × Breathing power indicator LED
- 4 × Battery level indicator LEDs

## Communication

- UART between ESP32 and Yahboom motor driver
- ESP32 WiFi Access Point for local web dashboard

---

# Pin Configuration

## Yahboom Motor Driver UART

| Signal | ESP32 GPIO |
| ------ | ---------- |
| Motor Driver RX to ESP32 TX | GPIO17 |
| Motor Driver TX to ESP32 RX | GPIO16 |

> Important: The Yahboom UART/I2C logic is 5V. ESP32 GPIOs are 3.3V only. Use a level shifter or voltage divider from Yahboom TX to ESP32 RX.

---

## MPU6050 Gyroscope

| Signal | ESP32 GPIO |
| ------ | ---------- |
| SDA | GPIO21 |
| SCL | GPIO22 |
| INT | GPIO35 |

---

## Ultrasonic Sensor

| Signal | ESP32 GPIO |
| ------ | ---------- |
| TRIG | GPIO27 |
| ECHO | GPIO26 |

> Important: If the ultrasonic sensor is HC-SR04, the ECHO pin is 5V. Use a voltage divider before connecting it to ESP32 GPIO26.

---

## Servo Motor

| Signal | ESP32 GPIO |
| ------ | ---------- |
| Servo Signal | GPIO25 |

> Power the servo from a suitable external 5V supply. Do not power the servo directly from the ESP32 3.3V pin.

---

## Battery Monitor

| Signal | ESP32 GPIO |
| ------ | ---------- |
| Battery ADC | GPIO34 |

> GPIO34 is input-only and is suitable for battery ADC sensing.

---

## LED Indicators

| Indicator | ESP32 GPIO |
| --------- | ---------- |
| Power Breathing LED | GPIO2 |
| Battery LED 1 | GPIO13 |
| Battery LED 2 | GPIO14 |
| Battery LED 3 | GPIO18 |
| Battery LED 4 | GPIO19 |

---

# Project Structure

```text
lib/
├── Actuators/
│   └── servo/
├── Communication/
│   ├── local_server/
│   ├── wifi_manager/
│   └── yahboom_motor/
├── Control/
│   ├── pid_controller/
│   └── robot_control/
├── Display/
│   └── led_indicator/
├── Pins/
├── Power/
│   └── battery_level/
├── Sensors/
│   ├── gyroscope/
│   └── ultrasonic/
├── System/
│   ├── error_handling/
│   ├── reset/
│   └── sleep_wake/

src/
└── main.cpp
```

---

# System Operation

## Power On

1. Power the ESP32, Yahboom motor driver, sensors, and motor supply.
2. The power indicator LED on GPIO2 starts breathing.
3. The battery level LEDs show the current battery percentage.
4. The ESP32 starts the WiFi access point for the web dashboard.
5. The Yahboom driver is configured through UART.
6. The ultrasonic sensor, servo, and gyroscope modules start running.
7. The car enters autonomous mode by default.

---

# Autonomous Driving Logic

The car is designed to behave like a simple self-driving robot car.

## Normal Forward Movement

When the front distance is safe:

- The servo remains centered.
- The car moves forward.
- Speed is controlled based on the measured distance.
- The farther the obstacle, the faster the car can move.
- The closer the obstacle, the slower the car moves.

---

## Gradual Speed Reduction

The car does not wait until it is very close to an obstacle before stopping.

Instead:

- At far distance, the car moves at normal speed.
- At medium distance, speed is reduced.
- At close distance, speed is reduced further.
- At stop distance, the car stops and begins obstacle scanning.

This makes the movement smoother and closer to a normal self-driving behavior.

---

## Obstacle Scan Behavior

When the car detects that an obstacle is too close:

1. The car stops.
2. The servo turns the ultrasonic sensor to the right.
3. The car reads the right-side distance.
4. The servo turns the ultrasonic sensor to the left.
5. The car reads the left-side distance.
6. The servo returns to center.
7. The car decides the best direction.

The system prefers a right turn when the right side is clear. If the right side is blocked but the left side is clearer, the car turns left.

---

## PID Speed Control

The PID speed control helps the car reduce speed smoothly as the obstacle distance changes.

The distance control uses:

- Setpoint distance
- Measured ultrasonic distance
- Error between desired and actual distance
- PID correction value

This correction is used to adjust the forward speed instead of suddenly stopping the car.

---

## PID Turn Control

The PID turn control uses gyroscope yaw feedback to make turning more stable.

During a turn:

- The system stores the starting yaw angle.
- A target yaw angle is selected.
- The car rotates until the target angle is reached.
- PID correction helps reduce overshoot.
- The car stops turning when the turn is complete.

This gives better control than turning for a fixed time only.

---

# Driving Modes

## Auto Mode

In auto mode, the robot controls itself using:

- Ultrasonic distance
- Servo scan result
- Gyroscope yaw
- PID control
- Battery safety condition

---

## Manual Mode

In manual mode, I can control the car from the web dashboard using:

- Forward
- Backward
- Left
- Right
- Stop

Manual mode is useful for testing motors, direction, and movement before enabling full autonomous behavior.

---

## Idle Mode

In idle mode:

- The motors are stopped.
- Sensors can still update.
- The web dashboard can still display system values.

---

# Web Dashboard

The ESP32 creates a local WiFi Access Point for monitoring and control.

## Access Point

```text
SSID: SmartCar_PID
Password: *****
IP Address: 192.168.4.1
```

## Dashboard Information

The dashboard displays:

- Current driving mode
- Front obstacle distance
- Right scan distance
- Left scan distance
- Current motor speed
- Left and right motor command
- Servo angle
- Battery voltage
- Battery percentage
- Gyroscope pitch
- Gyroscope roll
- Gyroscope yaw
- Motion status
- Yahboom motor driver status

## Dashboard Controls

The dashboard provides:

- Auto mode button
- Manual mode button
- Stop button
- Forward button
- Backward button
- Left turn button
- Right turn button

---

# Battery Monitoring

The battery voltage is read through the ESP32 ADC pin.

The system calculates:

- Battery voltage
- Battery percentage
- Low battery condition

The four battery LEDs show the approximate charge level:

| Battery Percentage | LED Display |
| ------------------ | ----------- |
| 0% - 24% | All battery LEDs OFF |
| 25% - 49% | LED1 ON |
| 50% - 74% | LED1 + LED2 ON |
| 75% - 94% | LED1 + LED2 + LED3 ON |
| 95% - 100% | LED1 + LED2 + LED3 + LED4 ON |

---

# Power Indicator

The power indicator LED is connected to GPIO2.

This LED is used only as a breathing status indicator. It is separate from the four battery level LEDs.

---

# Yahboom Motor Driver Notes

The Yahboom motor driver is controlled through UART in this project.

The ESP32 sends motor commands to the driver using serial command frames.

Example command types:

```text
$pwm:300,300,300,300#
$pwm:0,0,0,0#
$spd:300,300,300,300#
$spd:0,0,0,0#
```

For the L-shape geared 520 motors, the motor configuration used is:

```text
Motor Type: 1
Motor Phase: 40
Motor Line: 11
Wheel Diameter: 67.00
Deadzone: 1900
```

Only UART should be used for the Yahboom driver in this project. I2C should not be used at the same time as UART.

---

# Important Electrical Notes

- Do not power motors directly from the ESP32.
- Use a proper battery or power supply for the Yahboom motor driver.
- Use a common GND between the ESP32 and Yahboom driver.
- Use a level shifter or voltage divider from Yahboom TX to ESP32 RX.
- Do not connect 5V logic directly to ESP32 GPIO pins.
- Power the servo from a proper 5V supply.
- Add a capacitor near the servo power pins to reduce voltage drops.
- Add a large capacitor near the motor driver power input.
- Keep motor wires away from I2C and sensor signal wires where possible.
- If using HC-SR04, level-shift the ECHO signal before ESP32 GPIO26.

---

# PlatformIO Environment

```ini
[platformio]
description = Smart_Robotic_Car_With_PID_control

[env:esp32dev]
platform = espressif32@6.5.0
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    madhephaestus/ESP32Servo@^3.0.6

build_flags =
    -DARDUINO_ENABLE_PERIPHERAL_MANAGER=1
```

If the project uses the raw MPU6050 register-based driver, the Adafruit MPU6050 library is not required.

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

To monitor a specific port:

```bash
pio device monitor -p COM_PORT -b 115200
```

---

# Testing Guide

Before running the full autonomous system, each module should be tested separately.

## Recommended Test Order

1. GPIO2 power LED ON/OFF test
2. GPIO2 breathing LED test
3. Four battery level LED test
4. MPU6050 I2C scanner test
5. MPU6050 gyroscope data test
6. Servo motor test
7. Ultrasonic sensor test
8. Yahboom motor driver Type-C serial test
9. Yahboom motor driver ESP32 UART test
10. Web dashboard test
11. Full autonomous driving test

---

# Troubleshooting

## Motors Do Not Move

- Confirm the Yahboom motor driver has motor power, not only USB power.
- Confirm the motor battery voltage is suitable for the motors.
- Confirm the motors are connected to the correct output terminals.
- Confirm common GND between ESP32 and Yahboom driver.
- Confirm ESP32 TX goes to Yahboom RX and Yahboom TX goes to ESP32 RX through level shifting.
- Test the Yahboom board alone through Type-C serial first.
- Make sure no other module is using GPIO16 and GPIO17 UART pins.
- Try both `$pwm` and `$spd` commands depending on the driver firmware.

---

## Yahboom Does Not Reply Through UART

- Swap TX and RX temporarily to confirm pin labeling.
- Confirm baud rate is 115200.
- Confirm the board is in UART mode.
- Confirm I2C is not being used at the same time.
- Test using the Yahboom Type-C USB serial port first.

---

## Ultrasonic Reading Is Wrong

- Check TRIG and ECHO wiring.
- Confirm ECHO is level-shifted to 3.3V.
- Make sure the sensor is facing forward.
- Keep the sensor away from motor vibration.
- Make sure the servo is not moving during the exact distance capture.

---

## Servo Does Not Move

- Check servo signal pin GPIO25.
- Confirm servo has external 5V power.
- Connect servo GND to ESP32 GND.
- Avoid powering the servo from the ESP32 3.3V pin.

---

## MPU6050 Not Found

- Run an I2C scanner first.
- Expected address is usually `0x68`.
- Check SDA on GPIO21 and SCL on GPIO22.
- Confirm the module is powered correctly.
- Check GND connection.

---

## Web Page Does Not Open

- Connect to the ESP32 access point first.
- Use the correct IP address: `192.168.4.1`.
- Check that WiFi AP mode is enabled in the firmware.
- Restart the ESP32 and reconnect.

---

# Operating Guide

## Normal Operation

1. Place the robot on a safe open surface.
2. Power the motor driver and ESP32.
3. Connect to the ESP32 web dashboard if monitoring is needed.
4. Start in manual mode first to confirm motor directions.
5. Switch to auto mode.
6. The robot moves forward when the path is clear.
7. The robot slows down when approaching an obstacle.
8. The robot stops, scans, and turns when an obstacle is too close.

---

## First-Time Setup

1. Test the Yahboom motor driver alone through Type-C.
2. Confirm all four motors move correctly.
3. Test the ESP32 UART connection to the Yahboom driver.
4. Test the ultrasonic sensor.
5. Test the servo scan angles.
6. Test the MPU6050 yaw response.
7. Confirm battery LEDs and power LED.
8. Open the web dashboard.
9. Run the car in manual mode.
10. Enable auto mode only after all modules are confirmed.

---

## Safety Guide

- Keep the robot lifted during motor testing.
- Do not test full speed on a table edge.
- Do not place hands near wheels during motor test.
- Disconnect motor power when changing wiring.
- Use proper fuse protection for motor battery lines.
- Keep USB connected only for programming and monitoring.
- Make sure the robot has enough space before enabling auto mode.

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

- Add wheel encoder feedback for more accurate speed control
- Add true closed-loop motor speed PID using encoder data
- Add SD card logging for sensor and motor values
- Add OTA firmware update support
- Add mobile dashboard improvements
- Add obstacle map visualization
- Add automatic calibration mode
- Add current sensing for motor protection
- Add emergency stop button
- Add enclosure and PCB design

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
