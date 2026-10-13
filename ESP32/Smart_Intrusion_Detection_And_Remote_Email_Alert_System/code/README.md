# Smart Intrusion Detection and Remote Email Alert System

Standalone ESP32 intrusion detector with local audible alarm, Gmail SMTPS notification, secure local administration, and first-time Wi-Fi provisioning. It reuses the original ESP32 PCB mappings and contains no ESP-NOW or mesh operation.

## Verified PCB mapping

| Function | GPIO | Notes |
|---|---:|---|
| PIR input | 26 | HIGH is treated as motion; GPIO26 is RTC-capable. |
| Buzzer | 23 | Original firmware energised it HIGH. |
| Status/power/alarm LED | 2 | Original firmware used this active-HIGH LED. |
| Test button | 27 | Original active-LOW PCB button, with external bias assumed. |
| Battery ADC | 34 | Input-only, original divider scale 1.61. |
| Battery LEDs | 16–19 | Present on the original PCB; not repurposed by this application. |

## Operation

After the configured PIR warm-up, a HIGH PIR signal stable for the motion-confirmation period opens an intrusion event. The buzzer starts immediately, an email is queued on a separate FreeRTOS task, and the current alarm window runs without blocking sensing or the web server. At every alarm boundary, confirmed motion continues the same event and starts another cycle; confirmed clear state ends it.

The physical button has two distinct behaviours:

- A short press simulates a PIR intrusion using the complete alarm, log, and notification path.
- Holding it for five seconds opens the configuration access point for two minutes after initial setup. A long press does not simulate an intrusion.

During a configuration window, join the `IntrusionDetector-XXXX` AP. Its generated WPA2 password and `192.168.4.1` address are printed on the serial monitor. On a first boot without saved Wi-Fi, the AP remains active until Wi-Fi is configured.

## Local dashboard and security

Open the device IP address or `http://intrusion-detector.local` after connection. The first visitor must choose an administrator password of at least 12 characters; it is stored as a salted SHA-256 hash. API calls use a time-limited RAM session and CSRF token. Wi-Fi and Gmail passwords are never returned by the configuration API.

This is local HTTP, not HTTPS. Use it only on a trusted LAN and set a strong administrator password. The AP password is emitted only to serial output; treat that serial connection as privileged.

## Gmail

Enter the sender Gmail address, a Google App Password, recipient, and templates in the dashboard. The firmware connects to `smtp.gmail.com:465` by default and validates TLS with the Google Trust Services root. An accepted SMTP submission is not proof of inbox delivery.

## Build and flash

Install PlatformIO, then run from this folder:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run --target upload
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --baud 115200
```

The current target is `esp32dev` (ESP32-WROOM-class, 4 MB flash). Firmware output is `.pio/build/esp32dev/firmware.bin`.

## UI preview

Open [preview_UI/test.html](preview_UI/test.html) in a browser for a static preview of the dashboard. It is illustrative only; it does not connect to an ESP32.

## Commissioning checklist

1. Verify PIR output voltage never exceeds 3.3 V and is LOW while idle.
2. Confirm GPIO23 buzzer polarity and the GPIO2 LED polarity on the physical PCB.
3. Confirm a short button press triggers one alarm cycle and a five-second hold opens AP mode without an alarm.
4. Configure Wi-Fi, then Gmail App Password and recipient; use **Send test email**.
5. Test PIR HIGH for 20, 45, and 65 seconds, then test clear before and at an alarm boundary.
6. Measure sleep/idle power before enabling a battery-saver deep-sleep mode; PIR module, LED, regulator, and ADC divider currents are board-specific and must be measured.
