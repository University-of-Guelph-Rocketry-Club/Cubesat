# Hardware references

This repository is primarily software, but software needs accurate hardware references.

- `pinouts/` records STM32 and Pi connections and assigned signals.
- `wiring/` records device connections, voltage levels, interfaces, and notes.
- COTS avionics are documented as independent systems and are not prerequisites for the payload computer to boot, sense, or log.

Only approved connections belong here. Unknown values remain `TBD`. The STM32-to-Pi power and data boundary must be reviewed explicitly; the Pi must not become a prerequisite for STM32 operation.
