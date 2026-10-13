# Sensor acquisition and quality

The sample scheduler defaults to 60 seconds and accepts 5–86,400 seconds. A requested acquisition runs as a state machine; networking, indicators and the portal remain scheduled. Remote `*.read_now` returns `accepted` and completes after acquisition and local record storage. Failure to store is reported as failure, even when ADC acquisition completed.

## AM2301A

GPIO32 sends an open-drain low start pulse for at least 2 ms. Response capture checks the low/high handshake and 40 data pulses, with a 110 µs bound per pulse. The decoded checksum is the low byte of the first four bytes' sum. Humidity is unsigned tenths of percent; temperature uses the sign bit and tenths of a degree.

A wake/read frame is followed by a second frame after at least 2.2 seconds because this sensor family returns its prior conversion. Corrected readings use stored temperature/humidity offsets. Humidity correction is clamped to 0–100%. Protocol decoding is checked with manufacturer vectors; physical timing is not yet measured. [Aosong single-wire reference](https://www.aosong.com/userfiles/files/media/AM2301%E8%A7%84%E6%A0%BC%E4%B9%A6-%E8%8B%B1%E6%96%87%E7%89%88%EF%BC%8820210407%EF%BC%89.pdf).

On timeout, checksum/range failure or disable, current corrected values are null and `valid` is false. Last valid raw temperature/humidity, measurement age, failure count and acquisition uptime remain separately identified. Offset calibration never fabricates a missing reading.

## ADC channels

Each channel gathers 16 raw 12-bit samples across loop iterations. Sorting removes the two highest and two lowest; the remaining 12 are averaged. ESP-IDF calibration converts that same raw result to millivolts. Soil also uses an exponential moving average (25% new sample) for its relative index. Raw readings, calibrated millivolts and eFuse-calibration availability remain visible.

Raw values at ≤4 or ≥4,091, or ADC millivolts outside 150–2,450, are out of range. A result changing by at most one raw count for 60 acquisitions is `stuck_suspected`; this is a diagnostic heuristic, not proof of disconnection. Constant physical inputs can trigger it. Calibration commands require valid, non-saturated ADC acquisition no older than 120 seconds.

Soil uses either endpoint orientation and clamps its relative index to 0–100%. Endpoints less than 100 ADC counts apart are invalid. Voltage channels use independent divider/gain/offset/span settings. Missing or invalid calibration yields null engineering values. Battery percentage and soil VWC remain null; solar voltage does not establish charging state.

Sensor fault strings include `not_acquired`, `disabled`, `environment_timeout`, `environment_checksum_or_range`, `adc_out_of_range`, `adc_stuck_suspected`, `calibration_missing`, and `outside_validated_range`. Valid recovery frames or in-range ADC values clear their corresponding runtime conditions.
