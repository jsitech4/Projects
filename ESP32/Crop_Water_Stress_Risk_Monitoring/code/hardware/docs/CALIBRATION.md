# Calibration contract

All calibration resides in the atomic NVS configuration snapshot. Mutating commands require `expected_revision`; successful calibration writes increment both configuration and calibration revision. Invalid settings cannot replace valid saved values. Procedures work headlessly through MQTT and do not require the local connectivity portal.

| Group / field | Accepted values |
|---|---|
| `environment.temperature_offset` | −10 to +10 °C |
| `environment.humidity_offset` | −20 to +20 percentage points |
| `soil.dry`, `soil.wet` | 0–4,095 raw ADC counts, at least 100 apart |
| Power `top_ohm`, `bottom_ohm` | Both zero (uncommissioned), or both positive up to 100 MΩ |
| Power `gain` | 0.1–10 |
| Power `offset` | −10 to +10 V |
| Power `min_mv` | 150–2,400 mV |
| Power `max_mv` | 200–2,450 mV, greater than `min_mv` |
| Power `critical_v`, `low_v` | 0–1,000 V; critical ≤ low; zero disables that threshold |

For environment offsets, compare a valid sample against a reference instrument, then call `environment.set_offsets`. `reset_offsets` restores zero offsets and enabled acquisition.

For soil, acquire a reading with the probe in the recorded dry reference condition, then call `soil.capture_dry`. Repeat in the recorded wet reference condition with `soil.capture_wet`. A single endpoint can be saved while commissioning, but relative moisture remains invalid until both are valid. `soil.set_calibration` sets both numerical endpoints; `soil.validate_calibration` checks a pair without writing. The index is `100 × (filtered_adc − dry) / (wet − dry)`, clamped to 0–100. It is not VWC.

For battery/solar, enter the physically measured divider values and validated linear input span with `*.set_calibration`. Obtain a fresh reading, measure the source with a multimeter, then call `*.capture_reference` with `reference_v`. It computes:

```text
base_voltage = adc_mv / 1000 × (top_ohm + bottom_ohm) / bottom_ohm
gain = (reference_v − offset) / base_voltage
voltage = base_voltage × gain + offset
```

Capture rejects clipping, missing divider values, stale/busy acquisition and gain outside the accepted range. A single reference adjusts gain only; the full span must already have been electrically validated. Zener clipping cannot be corrected by gain. Battery and solar profiles remain independent.

`*.reset_calibration` restores the corresponding uncommissioned default profile, including its default thresholds. `config.restore_defaults` also resets calibration. Calibrations are included in sanitized configuration exports and telemetry revisions; credentials are not.
