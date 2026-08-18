# Optional STM32 SD-card wiring

HAB-001 currently assigns primary camera/data logging to the Pi Zero. This template remains for a possible independent STM32 SD-card logger and is not part of the initial architecture unless explicitly enabled.

| MCU connection | SD-card connection | Signal | Interface | Voltage | Notes |
| --- | --- | --- | --- | --- | --- |
| TBD | TBD | SCK | SPI | TBD | Bus assignment TBD |
| TBD | TBD | MOSI | SPI | TBD | Bus assignment TBD |
| TBD | TBD | MISO | SPI | TBD | Bus assignment TBD |
| TBD | TBD | CS | SPI | TBD | Chip-select TBD |
| TBD | TBD | Supply/ground | Power/reference | TBD | Level shifting TBD |

The storage test must verify file creation, append, flush, removal/reinsert behavior where appropriate, and visible failure status.
