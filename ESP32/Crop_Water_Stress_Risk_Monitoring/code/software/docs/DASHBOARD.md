# Crop Water Monitor dashboard

The single-device monitor lives at `/devices/CWS-5A170001` (using the registered device ID in production). Its desktop composition follows the supplied Crop Water Monitor reference: brand header, device identity and risk band, six measurements, a moisture/temperature chart with sensor health, and the hardware, activity, and configuration panels.

## Live data mapping

| Dashboard channel | Firmware source |
| --- | --- |
| Relative soil moisture | Capacitive soil sensor on GPIO34, reported as calibrated relative index |
| Air temperature and humidity | AM2301A on GPIO32 |
| Battery voltage | Battery divider on GPIO35 |
| Solar voltage | Solar divider on GPIO39 |

The browser reads `GET /devices/:id`, `GET /devices/:id/history`, and `GET /devices/:id/configuration` through the authenticated API. It refreshes every 15 seconds; the Refresh button runs the same data request immediately. MQTT remains server-side through the ingestion service.

## Configuration

Transmission interval maps to the device's supported `upload_s` configuration. Saving sends `config.patch` through `POST /devices/:id/configuration` with the current configuration revision, then waits for the device command response before reporting confirmation.

Data logging and the three alert thresholds are displayed as unavailable because the active ESP32 configuration contract does not advertise remotely writable controls for them. They are not simulated or persisted by the dashboard.

## Risk and unavailable data

Risk comes from the latest validated backend assessment. Without a valid configured threshold profile, the display says **Insufficient data**. Missing sensor readings remain unavailable; they are never replaced with zero or reference-image values.

## Verification

Run `npm run typecheck` in `apps/web`. Check the dashboard against the reference at a 1534 × 960 desktop viewport, then verify the responsive measurement strip and stacked panels at tablet and mobile widths.
