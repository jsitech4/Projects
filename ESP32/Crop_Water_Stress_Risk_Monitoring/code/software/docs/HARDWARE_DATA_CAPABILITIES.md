# Hardware data capabilities

The deployed firmware is the source of truth for available field data. The ESP32-WROOM-32U firmware currently reads the following installed channels:

| Channel | Hardware input | Platform data | Appropriate use | Important limit |
|---|---|---|---|---|
| Air temperature | AM2301A on GPIO32 | `temperature_c` | Heat context for water-stress assessment and trend review | A point air measurement; not canopy temperature. |
| Relative humidity | AM2301A on GPIO32 | `humidity_pct` | Atmospheric dryness context and trend review | Not rainfall, leaf wetness, or evapotranspiration. |
| Relative soil moisture | Analogue sensor on GPIO34 | `relative_moisture_pct`, raw ADC and mV | Plot-local wet/dry trend and a locally calibrated water-stress input | It is **not** volumetric water content (VWC). A dry/wet endpoint calibration is required. |
| Battery voltage | Analogue divider on GPIO35 | `voltage_v`, low/critical flags, raw ADC and mV | Power-health and monitoring reliability | Percentage/state-of-charge is not calculated. |
| Solar voltage | Analogue divider on GPIO39 | `voltage_v`, low/critical flags, raw ADC and mV | Solar-input availability and power diagnostics | Voltage alone does not prove charging power or current. |

The device also reports data quality, calibration validity, sensor fault codes, acquisition times, Wi-Fi/MQTT/TLS state, RSSI, storage queue state, device health, configuration revisions, and optional LCD/encoder state. These are fetched through the versioned MQTT telemetry, status, configuration, capabilities, event, and command-response topics.

## What this hardware does not fetch

It does not measure rainfall, irradiance/PAR, wind, soil temperature, soil pH, EC, nutrient concentration, water flow, leaf wetness, crop growth stage, yield, or harvest maturity. Those values must remain unavailable unless compatible physical sensors or external sources are added and the firmware contract is extended.

## Crop water-stress interpretation

The platform only creates a water-stress classification after a crop profile with locally validated thresholds is assigned to the plot. Profiles operate on the calibrated relative soil-moisture index and must identify their rule version. If a profile, calibration, current measurement, or valid environment reading is unavailable, the risk is reported as `UNKNOWN` rather than inferred.
