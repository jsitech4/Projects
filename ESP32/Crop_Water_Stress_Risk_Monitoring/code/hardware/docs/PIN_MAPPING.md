# Manufactured PCB pin mapping

| Symbol | GPIO | Mode |
|---|---:|---|
| `DHT_DATA` | 32 | Single-wire open-drain start / input response |
| `SOIL_SENSOR` | 34 | ADC1 input |
| `BATTERY_SENSE` | 35 | ADC1 input |
| `SOLAR_SENSE` | 39 | ADC1 input |
| `LCD_SDA`, `LCD_SCL` | 21, 22 | Optional I²C |
| `ENCODER_CLK`, `ENCODER_DT`, `ENCODER_SW` | 25, 26, 27 | Explicitly enabled inputs with pull-ups |
| `STATUS_LED` | 2 | LEDC channel 0 |
| `BUZZER` | 4 | Disabled input by default; verified active/passive output profile |

`lib/Board/Pins.h` is authoritative; `Pins.cpp` supplies the exclusion invariant. Native tests check every assignment. OLED names have been replaced by LCD names without changing the bus pins.

GPIO14, GPIO12, GPIO33 and GPIO16 are not initialized or driven. The firmware has no general GPIO command surface. GPIO34/35/39 are input-only pins; their circuitry cannot rely on internal pull-ups.
