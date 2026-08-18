# Shared definitions

This directory contains small definitions that must mean the same thing in STM32 firmware, Pi Zero logging software, and ground software.

- `telemetry/` is the canonical logical telemetry record and field-validity rules.
- `protocols/` records STM32-to-Pi and radio protocol boundaries and versioning expectations.

Keep device drivers and mission-specific behavior out of this directory.
