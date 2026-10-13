# Hardware and electrical acceptance

The build targets the original ESP32-WROOM-32U with a 4 MiB flash layout. The module requires a correctly connected external antenna. Physical flash identity, PCB revision, supply integrity and fitted components are commissioning measurements, not properties established by compilation.

All three analog inputs use ADC1 so Wi-Fi does not reserve their converter. The firmware uses the ESP-IDF calibration API and marks whether eFuse calibration was available. Its conservative accepted ADC range is 150–2,450 mV. This follows the original ESP32's documented calibrated range; it is not permission to exceed the pin's electrical rating. [Espressif ADC reference](https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/adc.html).

Battery and solar top/bottom resistances default to zero. Record fitted values and sweep each divider against a multimeter. Set `max_mv` below the observed onset of Zener clipping, including tolerances and temperature effects. A rail reading, ADC clipping or input outside the validated span yields null voltage. No current or charging state is derived from solar voltage.

AM2301A requires a suitable pull-up and ESP32-safe logic voltage. Its manufacturer specifies a single-wire digital protocol. Verify the exact sensor marking, wiring and power arrangement against the [Aosong AM2301A manual](https://uelectronics.com/wp-content/uploads/2022/05/AM2301A-Datasheet.pdf).

GPIO2 is a boot strapping pin. GPIO4 is **not** a strapping pin on the original ESP32; the board's buzzer load and driver still require verification. Confirm polarity and transistor/driver current limits before enabling the buzzer. [ESP32 datasheet](https://documentation.espressif.com/esp32_datasheet_en.html).

LCD support assumes the common PCF8574 backpack mapping: P0=RS, P1=RW, P2=E, P3=backlight, P4–P7=data. Confirm that mapping, address and I²C pull-up voltage; 5 V pull-ups must not reach the ESP32 pins. Detection confirms I²C acknowledgement, not the identity of an arbitrary responding device. Encoder installation is explicitly configured and never inferred from floating pin levels.

Outstanding physical checks: ADC transfer curves and clipping, sensor timing, load polarity, LCD levels, boot behavior with fitted loads, radio coexistence, TLS peak heap, power-loss storage behavior, and power supply stability. No resistor value or bench measurement has been assumed.
