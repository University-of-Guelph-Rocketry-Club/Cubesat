# Flight software lifecycle

The intended onboard lifecycle is:

```text
BOOT
 ↓
SELF TEST
 ↓
PREFLIGHT
 ↓
ASCENT
 ↓
DESCENT
 ↓
LANDED
```

## Current implementation

[`onboard/stm32/apps/flight/main.c`](../onboard/stm32/apps/flight/main.c) initializes the board, creates the FreeRTOS flight tasks, and starts the scheduler. [`flight_tasks.c`](../onboard/stm32/apps/flight/flight_tasks.c) defines the initial task boundaries. Reusable state sequencing remains in [`state_machine.c`](../onboard/stm32/modules/state_machine/state_machine.c). Hardware access, detector thresholds, RTOS resource sizing, radio transport, and Pi transport remain explicit TODOs until their hardware and tests are approved.

## Planned behavior

- **BOOT:** initialize the STM32 application and establish a known context.
- **SELF TEST:** check required buses and report failures visibly.
- **PREFLIGHT:** sample installed devices, acquire GPS when available, and expose readiness without inventing data.
- **ASCENT/DESCENT/LANDED:** transition only from tested detector inputs, while continuing sensing, Pi forwarding, telemetry, and health checks.
- **FAULT:** expose critical failure and preserve whatever safe sensing/telemetry behavior the approved policy allows.

State sequencing is present, but launch, descent, and landing detectors are not implemented. The initial application must not claim those events from invented thresholds.

## FreeRTOS execution

The flight application separates sensor acquisition, GPS, mission-state ownership, telemetry, Pi transport, and system-health/watchdog work into tasks. Drivers expose bounded calls and do not create private tasks. Queues and notifications carry data or wake tasks; event groups may represent readiness and heartbeat state. See [rtos-architecture.md](rtos-architecture.md).
