# Headless and optional hardware operation

LCD and encoder settings default disabled. Boot, local credentials, network setup, calibration, MQTT, storage and physical-service recovery operate without either peripheral.

The administrator uses the five-minute setup AP for connectivity. Calibration and operating preferences use authenticated MQTT commands. The backend can be a local development broker; no external website or database is required for acquisition and storage.

Enable installed optional hardware with a revision-checked `config.patch`:

```json
{"patch":{"lcd":{"enabled":true,"address":39},"encoder":{"enabled":true}}}
```

Address 39 is decimal 0x27; it is a software profile default, not a detected board fact. The configured address alone is probed with a bounded I²C timeout. Missing LCD hardware raises `lcd_missing` and leaves core operation running. `lcd.set_enabled` remains available for profile changes; other LCD controls return `unsupported_hardware` when absent. Capability publication omits those controls while absent.

The display has environment/soil, power and system pages. A quadrature transition table requires four valid transitions per detent; the switch cycles pages with 40 ms debounce. Backlight timeout follows encoder activity or notices. Controls do not edit security or connectivity settings.

LED priorities are fault, warning, provisioning, online and offline/connecting state. Breathing is an explicit preference after polarity verification. Buzzer defaults disabled and never activates for more than three seconds per operation. Mute lasts 1–86,400 seconds; disabling and muting stop an active sound.
