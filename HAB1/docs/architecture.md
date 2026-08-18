# Architecture

The HAB is one vehicle with independent student-owned subsystems and two onboard computers. The initial software architecture keeps the critical flight path on the STM32:

```text
Sensors / GPS / Power
          ↓
STM32 Flight Controller
    ├──────────────→ Radio Telemetry ──→ Ground Station
    │
    └──────────────→ Pi Zero Data Logger ──→ Camera + Local Storage
```

## Responsibilities

- `onboard/stm32/apps/flight/` is the final C/FreeRTOS flight application.
- `onboard/stm32/apps/bringup/` gives each STM32 subsystem a standalone student application.
- `onboard/stm32/modules/` contains reusable drivers and services linked by both application types.
- `onboard/pi-zero/` contains standalone bring-up workspaces and the final camera/data-logger application.
- `onboard/integration/` assembles both computers for HAB-001 without copying subsystem code.
- `shared/telemetry/` defines the canonical C logical record used by STM32, Pi, and ground software.
- `shared/protocols/` owns versioned STM32-to-Pi and radio framing.
- `ground/` contains laptop-side receiving, visualization, and recovery software.
- `tools/` contains analysis, parsing, decoding, and simulation utilities.
- `tests/` organizes unit, integration, hardware, and fixture-based validation.
- `config/` stores mission-specific values such as `HAB-001` rates and enabled sensors.
- `hardware/` records software-relevant pins, interfaces, voltage levels, and wiring decisions.
- `data/` contains only small examples; real flight data should remain in external or ignored storage.
- `docs/` explains how to build, understand, test, and operate the software.

## Boundaries

STM32 code should be deterministic C, readable, conservative with memory, and organized as statically allocated FreeRTOS tasks. Device-specific details belong behind small interfaces or measurement functions. Tasks communicate through bounded queues, notifications, or event groups; interrupt handlers remain short. The Pi and ground software may use richer Linux libraries.

The STM32 must continue sensing, state handling, health monitoring, and primary telemetry if the Pi fails. The Pi preserves received records and camera data but is not a flight-control dependency.

MissileWorks RRC-series altimeters and Eggtimer/Eggfinder equipment are independent COTS avionics and tracking/recovery systems. The payload computer does not depend on them to boot, collect scientific data, or log onboard data.

FreeRTOS is selected for the STM32, but the project does not require microservices, dependency-injection frameworks, an elaborate abstraction layer, a binary protocol, or a graphical dashboard. Additional complexity should be added only when a mission need justifies it.
