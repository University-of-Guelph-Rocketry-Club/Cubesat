# HAB-001 Software

Software for the University of Guelph Rocketry Club's student-built High-Altitude Balloon project.

The HAB is one team vehicle with multiple student-owned subsystems. HAB-001 uses an STM32 running C and FreeRTOS as the primary flight controller, plus a Raspberry Pi Zero with a Pi Camera as an onboard data logger. Environmental sensors, GPS, telemetry, and power monitoring connect to the STM32 as their hardware is selected.

## Current status

This repository is in the architecture and onboarding stage. It contains separate subsystem bring-up workspaces, final integration applications, shared interfaces, sample data, and test guidance. It is not mission-ready; the exact STM32 family, sensors, radio, Pi model, camera model, and physical STM32-to-Pi link remain to be selected.

## Repository map

```text
onboard/   → STM32 firmware, Pi Zero software, and final integration
ground/    → software intended to run on a laptop or ground computer
shared/    → telemetry and protocols shared by onboard and ground software
tools/     → developer, parsing, decoding, and simulation utilities
tests/     → software, hardware, and test-data organization
config/    → mission and sensor configuration references
hardware/  → pinouts and software-relevant wiring documentation
docs/      → software-focused project documentation
data/      → small example datasets; large raw data stays outside Git
```

## Quick start

```bash
git clone https://github.com/University-of-Guelph-Rocketry-Club/Cubesat.git
cd Cubesat/HAB1
```

Then read:

1. [Getting started](docs/getting-started.md)
2. [Architecture](docs/architecture.md)
3. The README for the subsystem you want to learn in `onboard/stm32/apps/bringup/`

The bring-up applications give each subsystem a standalone student workspace. Reusable code belongs in `onboard/stm32/modules/` or `onboard/pi-zero/modules/`, and the final applications link those same modules rather than copying code.

## Documentation

- [Architecture](docs/architecture.md)
- [Flight software lifecycle](docs/flight-software.md)
- [FreeRTOS architecture](docs/rtos-architecture.md)
- [Telemetry format](docs/telemetry.md)
- [Testing progression](docs/testing.md)
- [Mission flow](docs/mission-flow.md)
- [Getting started](docs/getting-started.md)
- [Contribution guide](CONTRIBUTING.md)

## Contributions

Use a focused feature or fix branch and keep pull requests small. See [CONTRIBUTING.md](CONTRIBUTING.md) for the expected description and testing information.
