# Smart Intrusion Detection and Remote Email Alert System

Standalone ESP32 intrusion detector with local audible alarm, Gmail SMTPS notification, secure local administration, and first-time Wi-Fi provisioning. It reuses the original ESP32 PCB mappings and contains no ESP-NOW or mesh operation.

## Verified PCB mapping

| Function           |  GPIO | Notes                                                        |
| ------------------ | ----: | ------------------------------------------------------------ |
| PIR input          |    26 | HIGH is treated as motion; GPIO26 is RTC-capable.            |
| Buzzer             |    23 | Original firmware energised it HIGH.                         |
| Status LED         |     2 | Active-HIGH patterned status, alert, fault, and network LED. |
| Test button        |    27 | Original active-LOW PCB button, with external bias assumed.  |
| Battery ADC        |    34 | Input-only, original divider scale 1.61.                     |
| Battery gauge LEDs | 16–19 | Active-HIGH 25%, 50%, 75%, and 100% battery-level bars.      |

## Operation

After the configured PIR warm-up, a HIGH PIR signal stable for the motion-confirmation period opens an intrusion event. The buzzer starts immediately, an email is queued on a separate FreeRTOS task, and the current alarm window runs without blocking sensing or the web server. At every alarm boundary, confirmed motion continues the same event and starts another cycle; confirmed clear state ends it.

The device retains only the most recent eight completed intrusion events. This rolling history automatically overwrites the oldest event, keeping persistent storage use predictable and avoiding unnecessary flash growth.

## LED indicators

GPIO2 is the single status LED. Its patterns have priority in this order: intrusion, critical battery, low battery, system fault, Wi-Fi loss, internet loss, PIR warm-up, and configuration mode.

| GPIO2 pattern          | Meaning                                           |
| ---------------------- | ------------------------------------------------- |
| Three rapid flashes    | Intrusion alarm is active.                        |
| Four short flashes     | Critical battery.                                 |
| Two medium flashes     | Low battery.                                      |
| Two fault flashes      | A persistent firmware/system fault.               |
| One long flash         | Wi-Fi is disconnected.                            |
| Two very short flashes | Wi-Fi is connected but the internet check failed. |
| One brief heartbeat    | Powered, healthy, and internet-reachable.         |

GPIO16, GPIO17, GPIO18, and GPIO19 form a cumulative battery bar: one, two, three, or four LEDs represent 1–25%, 26–50%, 51–75%, and 76–100% respectively. The gauge is off when battery monitoring is disabled or the estimated level is empty. Internet reachability is checked with a short TCP probe every 30 seconds; it confirms an external route, not every online service.

## Reliability and automated checks

The ESP32 loop task is protected by a 10-second task watchdog. It is fed only after the complete control loop has run, so a stalled loop resets the device instead of leaving it unresponsive. A reset caused by the watchdog is retained as a fault indication after restart.

The native scenario suite covers battery-gauge boundaries, status-LED priority and pulse timing, and both repeat-alarm paths. Run `tools\run_scenario_tests.bat` to execute it automatically with installed Visual Studio C++ Build Tools. On machines using GCC or MinGW, `platformio test -e native` also works. The production firmware build remains `platformio run -e esp32dev`.

The physical button has two distinct behaviours:

- A short press simulates a PIR intrusion using the complete alarm, log, and notification path.
- Holding it for five seconds opens the configuration access point for two minutes after initial setup. A long press does not simulate an intrusion.

During a configuration window, join the `IntrusionDetector-XXXX` AP. Its generated WPA2 password and `192.168.4.1` address are printed on the serial monitor. On a first boot without saved Wi-Fi, the AP remains active until Wi-Fi is configured.

## Local dashboard and security

Open the device IP address or `http://intrusion-detector.local` after connection. The first visitor must choose an administrator password of at least 12 characters; it is stored as a salted SHA-256 hash. API calls use a time-limited RAM session and CSRF token. Wi-Fi and Gmail passwords are never returned by the configuration API.

This is local HTTP, not HTTPS. Use it only on a trusted LAN and set a strong administrator password. The AP password is emitted only to serial output; treat that serial connection as privileged.

## Gmail

Enter the sender Gmail address, a Google App Password, one to five recipient addresses, and an alert message in the dashboard. Put multiple addresses in the **Recipient email** field separated with commas, semicolons, or line breaks. Addresses are checked, duplicates are removed, and each message uses private SMTP envelope recipients so recipients do not see one another. The firmware connects to `smtp.gmail.com:465` by default and validates TLS with the Google Trust Services root. An accepted SMTP submission is not proof of inbox delivery.

The email subject is generated as `SECURITY ALERT - <device name>`. The alert message is sent exactly as written, followed automatically by the device location, event ID, and NTP-synchronized Nigeria time. The occurrence timestamp is captured when the intrusion begins, so a delayed delivery still reports the original event time.

## Time in Nigeria

The default timezone is `WAT-1` (West Africa Time, UTC+1), which is the POSIX timezone string for Nigeria. After Wi-Fi connects, the ESP32 synchronizes with multiple NTP servers and keeps local event timestamps and email times in WAT. If the connection is lost, it derives time from the last trusted NTP reading and elapsed ESP32 uptime until NTP is available again. The dashboard displays the device's synchronized time; until its first successful NTP sync it reports that the time is unavailable. Keep the timezone set to `WAT-1` for operation in Nigeria.

## Battery-divider calibration

The battery divider scale converts the ESP32 ADC reading on GPIO34 into the actual battery voltage:

```text
battery voltage = ADC-pin voltage × battery divider scale
```

The default scale is `1.61`, taken from the original PCB. To calibrate it, measure the battery with a multimeter (`Vbattery`) and compare it with the voltage shown in the dashboard (`Vshown`). Then save:

```text
new divider scale = current divider scale × (Vbattery ÷ Vshown)
```

For example, if the current scale is `1.61`, the multimeter reads `4.00 V`, and the dashboard reports `3.80 V`, use `1.61 × (4.00 ÷ 3.80) = 1.695`. Recheck the displayed value after saving; calibrate at a normal battery operating voltage.

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
4. Configure Wi-Fi, Gmail App Password, and up to five recipients; use **Send test email** and confirm delivery to each inbox.
5. Test PIR HIGH for 20, 45, and 65 seconds, then test clear before and at an alarm boundary.
6. Measure sleep/idle power before enabling a battery-saver deep-sleep mode; PIR module, LED, regulator, and ADC divider currents are board-specific and must be measured.

## Security and local configuration

Never commit populated `.env` files, Wi-Fi credentials, passwords, API keys, SMTP credentials, broker tokens, private certificates, or production connection strings. Keep them in an ignored local configuration file or approved secret store, and commit only a redacted `.env.example` when configuration is needed. Replace development access-point credentials before deployment and limit local administration to trusted networks.

# Author

Egbe Raymond

Electrical / Electronic Engineer
Embedded Systems Developer
Full Stack Developer

---


## Engineering asset map and license scope

The editable electrical package is under `hardware/` as a KiCad project,
schematic, and PCB, with project library/3D materials. `documentation/`
contains the schematic and block diagrams, ESP32 reference, and board images;
`production/` contains the BOM, Gerbers, positions, designators, and IPC
netlist. Inspect and modify the source project, then regenerate manufacturing
outputs for a replacement board.

The PlatformIO/Arduino code in `code/` targets `esp32dev` at 115200 baud and
also declares a native test environment. Its modules separate PIR/button
input, buzzer/LED state, Wi-Fi and email delivery, intrusion policy and
controller, event logging/configuration, web administration, battery handling,
and error management. SMTP credentials, Wi-Fi credentials, recipient data, and
event records are deployment secrets or personal data and must not be committed.

The asset-type table below covers original firmware, KiCad files, original images/exports, and project
documentation. ESP32 and sensor material, third-party libraries, mail-service
terms, vendor assets, and trademarks are not relicensed. Consult the
[license scope guide](../../LICENSE-SCOPE.md) and do not treat this prototype
as a certified life-safety or security-response system.

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
