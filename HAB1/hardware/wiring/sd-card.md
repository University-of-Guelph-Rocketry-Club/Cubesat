# Optional STM32 SD-card wiring

URC-43 proposes an independent STM32 logger on a dedicated SPI bus. Its
[interface proposal](../../docs/sd-logging.md) records the difference between
that assignment and the initial Pi-first architecture. No pins or supply
connections are selected by that proposal.

HAB-001 currently assigns primary camera/data logging to the Pi Zero. This template remains for a possible independent STM32 SD-card logger and is not part of the initial architecture unless explicitly enabled.

| MCU connection | SD-card connection | Signal | Interface | Voltage | Notes |
| --- | --- | --- | --- | --- | --- |
| TBD | TBD | SCK | SPI | TBD | Bus assignment TBD |
| TBD | TBD | MOSI | SPI | TBD | Bus assignment TBD |
| TBD | TBD | MISO | SPI | TBD | Bus assignment TBD |
| TBD | TBD | CS | SPI | TBD | Chip-select TBD |
| TBD | TBD | Supply/ground | Power/reference | TBD | Level shifting TBD |

The storage test must verify file creation, append, flush, removal/reinsert behavior where appropriate, and visible failure status.
