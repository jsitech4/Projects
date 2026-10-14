# Solar Powered Egg Incubator System

## Project Overview

This project is a solar-powered egg incubator system built around the ESP32-WROOM-32 / ESP32-WROOM-32U microcontroller. I designed the system to monitor and control the important conditions required inside the incubator, such as temperature, humidity, battery level, solar input, and internal distance/level monitoring through an ultrasonic sensor.

The system uses a modular firmware structure so that every major function is separated into its own module. This makes the code easier to maintain, test, and expand in the future.

The project also includes a simple local web dashboard hosted directly by the ESP32. Through this dashboard, I can view live sensor values and control some system outputs from a phone or laptop connected to the ESP32 access point.

---

## Main Features

- ESP32-WROOM-32 / ESP32-WROOM-32U based control system
- Solar and battery voltage monitoring
- AHT20 temperature and humidity monitoring
- DS3231 RTC time/date monitoring
- Ultrasonic distance monitoring
- Heater control through relay
- Spinner control through relay
- Humidifier control through relay
- Buzzer alert system
- 128x64 ST7920 LCD display using software SPI
- Rotary encoder for user interaction and LCD wake activity
- LCD auto-sleep function to reduce power consumption
- Local web dashboard hosted by the ESP32
- Modular PlatformIO project structure

---

## Controller Used

The controller used for this project is:

```text
ESP32-WROOM-32 / ESP32-WROOM-32U
```

The ESP32-WROOM-32U version uses an external antenna, while the ESP32-WROOM-32 uses the onboard PCB antenna. Both versions use the same GPIO mapping, so the firmware and pin assignment remain the same.

---

## Pin Assignment

| Function | ESP32 GPIO | Description |
|---|---:|---|
| Battery Level | GPIO34 | ADC input for battery voltage divider |
| Solar Level | GPIO35 | ADC input for solar voltage divider |
| Ultrasonic TRIG | GPIO27 | Ultrasonic trigger pin |
| Ultrasonic ECHO | GPIO26 | Ultrasonic echo pin |
| Rotary Encoder A | GPIO32 | Encoder channel A |
| Rotary Encoder B | GPIO33 | Encoder channel B |
| Rotary Encoder SW | GPIO25 | Encoder push button / LCD wake activity |
| Buzzer | GPIO4 | Buzzer output |
| Heater Relay | GPIO16 | Heater relay control |
| Spinner Relay | GPIO17 | Spinner relay control |
| Humidifier Relay | GPIO18 | Humidifier relay control |
| LCD CS | GPIO13 | ST7920 software SPI CS |
| LCD CLK | GPIO14 | ST7920 software SPI clock |
| LCD MOSI | GPIO23 | ST7920 software SPI data |
| I2C SDA | GPIO21 | AHT20 and RTC SDA |
| I2C SCL | GPIO22 | AHT20 and RTC SCL |

---

## I2C Devices

The following devices share the same I2C bus:

```text
SDA = GPIO21
SCL = GPIO22
```

| Device | Purpose |
|---|---|
| AHT20 | Temperature and humidity sensor |
| DS3231 RTC | Real time clock |

Both devices should be powered from 3.3V and connected to the common ground of the ESP32.

---

## LCD Interface

The LCD used in this project is a 128x64 ST7920 display.

This LCD is controlled using software SPI through the U8g2 library.

```text
LCD CS   = GPIO13
LCD CLK  = GPIO14
LCD MOSI = GPIO23
```

I used software SPI because the ST7920 does not behave like a normal hardware SPI display. This makes the display more stable and avoids unnecessary SPI conflicts.

---

## Relay Outputs

The system controls three relay-based outputs:

| Output | Purpose |
|---|---|
| Heater Relay | Turns the heating element ON/OFF |
| Spinner Relay | Turns the egg turning/spinner mechanism ON/OFF |
| Humidifier Relay | Turns the humidifier ON/OFF |

The relay modules are assumed to be active LOW. This means the relay is OFF when the ESP32 pin is HIGH and ON when the ESP32 pin is LOW.

---

## Local Web Dashboard

The ESP32 hosts a simple local dashboard through Wi-Fi Access Point mode.

Default access point details:

```text
Wi-Fi Name: Egg_Incubator
Password: ********
Dashboard IP: 192.168.4.1
```

For security, I do not expose the real password in this README. The actual password is defined inside the Wi-Fi manager module in the firmware.

The dashboard shows live values such as:

- Temperature
- Humidity
- Battery voltage
- Battery percentage
- Solar voltage
- Solar percentage
- Ultrasonic distance
- LCD state
- Heater state
- Spinner state
- Humidifier state
- RTC date and time

The dashboard also provides control buttons for:

- Heater toggle
- Spinner toggle
- Humidifier toggle
- Buzzer beep
- LCD wake
- LCD sleep

---

## How to Operate the System

### 1. Power the System

I power the ESP32 and all connected modules using the correct regulated supply. Before powering the full system, I confirm that the ESP32 receives 3.3V logic-level signals and that all relay modules, sensors, and display modules share a common ground.

Important checks before power-up:

- Battery voltage divider must not exceed 3.3V at the ESP32 ADC pin
- Solar voltage divider must not exceed 3.3V at the ESP32 ADC pin
- Ultrasonic ECHO must be level-shifted to 3.3V if using a 5V HC-SR04 sensor
- Relay module input logic must match the firmware active-LOW configuration
- I2C devices must be connected to GPIO21 and GPIO22

---

### 2. Upload the Firmware

I open the project in Visual Studio Code using PlatformIO.

Then I connect the ESP32 board and upload the firmware.

Typical PlatformIO command:

```bash
pio run --target upload
```

To monitor the serial output:

```bash
pio device monitor
```

The monitor baud rate should be set to:

```text
115200
```

---

### 3. Connect to the ESP32 Dashboard

After the ESP32 boots, it creates a Wi-Fi access point.

I connect my phone or laptop to:

```text
Wi-Fi Name: Egg_Incubator
Password: ********
```

Then I open a browser and visit:

```text
http://192.168.4.1
```

From there, I can view the live dashboard and control the system outputs.

---

### 4. LCD Operation

The LCD displays the main system information, including:

- Temperature
- Humidity
- Battery voltage
- Solar voltage
- Ultrasonic distance
- Heater state
- Spinner state
- Humidifier state
- RTC date/time

The LCD automatically sleeps after a period of inactivity to reduce power consumption.

The LCD can wake when there is user activity from the rotary encoder or when the wake button is pressed from the dashboard.

---

### 5. Rotary Encoder Operation

The rotary encoder is used as a user input device. It can detect rotation and button press events.

In the current structure, the encoder also acts as user activity for waking the LCD.

Encoder pins:

```text
A  = GPIO32
B  = GPIO33
SW = GPIO25
```

---

### 6. Sensor Monitoring

The firmware continuously updates sensor readings in a non-blocking way.

| Sensor | Update Purpose |
|---|---|
| AHT20 | Reads temperature and humidity |
| Battery ADC | Reads battery voltage and percentage |
| Solar ADC | Reads solar voltage and percentage |
| Ultrasonic | Reads distance in centimeters |
| RTC | Updates date and time |

The web dashboard updates these values automatically through the `/api` route.

---

### 7. Relay Control

The heater, spinner, and humidifier relays can be controlled from the dashboard.

Each relay module exposes:

- `setState(true)` to turn ON
- `setState(false)` to turn OFF
- `toggle()` to change state
- `isOn()` to check current state

At startup, all relay outputs are kept OFF.

---

## Firmware Module Structure

The firmware is arranged in modules so each part of the system has its own responsibility.

Typical modules include:

```text
Pins
battery_level
solar_level
temp_hum
ultrasonic
rotary_encoder
lcd_screen
rtc_clock
heater
spinner
humidifier
buzzer
wifi_manager
web_dashboard
sleep_wake
error_handling
reset
led_indicator
```

This modular structure makes it easier for me to debug one section at a time instead of mixing all logic inside `main.cpp`.

---

## Dashboard Routes

| Route | Purpose |
|---|---|
| `/` | Main dashboard page |
| `/api` | JSON sensor/status API |
| `/toggle?dev=heater` | Toggle heater relay |
| `/toggle?dev=spinner` | Toggle spinner relay |
| `/toggle?dev=humidifier` | Toggle humidifier relay |
| `/beep` | Trigger buzzer beep |
| `/lcd?wake=1` | Wake LCD |
| `/lcd?wake=0` | Sleep LCD |

---

## Future Improvements

Possible future improvements for this project include:

- Add automatic incubation control logic
- Add temperature setpoint adjustment from the rotary encoder
- Add humidity setpoint adjustment from the dashboard
- Add EEPROM/Preferences storage for saved settings
- Add internet-based dashboard support using MQTT, Firebase, Blynk, or a custom API
- Add alarm logging
- Add SD card logging
- Add OTA firmware update support
- Add calibration menu for battery and solar readings

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
