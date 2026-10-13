# Calibration contract

All calibration resides in the atomic NVS configuration snapshot. Successful calibration writes increment both configuration and calibration revision. Invalid settings cannot replace valid saved values.

Calibration is available only when the privileged fixed password is used at the local portal sign-in screen. That verifier is immutable and independent of the administrator password; changing the administrator password neither exposes nor replaces it. The calibration navigation entry and values are absent from an ordinary administrator session. Privileged access ends with logout, portal expiry, or restart. MQTT telemetry and commands can report sensor status, but cannot read or change calibration settings.

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

For environment offsets, compare a valid sample against a reference instrument, then enter the offsets in the Developer calibration page.

For soil, acquire a fresh reading with the probe in the recorded dry reference condition and enter that ADC value. Repeat in the recorded wet reference condition. A single endpoint can be saved while commissioning, but relative moisture remains invalid until both are valid. The page accepts either orientation and requires an endpoint span of at least 100 ADC counts. The index is `100 × (filtered_adc − dry) / (wet − dry)`, clamped to 0–100. It is not VWC.

For battery/solar, enter the physically measured divider values, validated linear ADC span, gain, offset and warning thresholds in the Developer calibration page. Obtain a fresh reading and measure the source with a multimeter before establishing the gain. The calculation is:

```text
base_voltage = adc_mv / 1000 × (top_ohm + bottom_ohm) / bottom_ohm
gain = (reference_v − offset) / base_voltage
voltage = base_voltage × gain + offset
```

The page rejects incomplete divider values, an invalid ADC span, gain outside the accepted range, and invalid threshold ordering. A single reference adjusts gain only; the full span must already have been electrically validated. Zener clipping cannot be corrected by gain. Battery and solar profiles remain independent.

Calibration resets and remote calibration commands are disabled. Configuration resets preserve the existing calibration snapshot. Telemetry reports calibration revisions and measurement quality, but calibration values are not included in remote configuration exports.
