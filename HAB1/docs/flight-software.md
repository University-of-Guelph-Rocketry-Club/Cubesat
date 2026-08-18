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

[`onboard/stm32/apps/flight/main.c`](../onboard/stm32/apps/flight/main.c) is a readable C superloop skeleton. The reusable state sequencing is implemented in [`state_machine.c`](../onboard/stm32/modules/state_machine/state_machine.c). Hardware access, detector thresholds, radio transport, and Pi transport remain explicit TODOs until their hardware and tests are approved.

## Planned behavior

- **BOOT:** initialize the STM32 application and establish a known context.
- **SELF TEST:** check required buses and report failures visibly.
- **PREFLIGHT:** sample installed devices, acquire GPS when available, and expose readiness without inventing data.
- **ASCENT/DESCENT/LANDED:** transition only from tested detector inputs, while continuing sensing, Pi forwarding, telemetry, and health checks.
- **FAULT:** expose critical failure and preserve whatever safe sensing/telemetry behavior the approved policy allows.

State sequencing is present, but launch, descent, and landing detectors are not implemented. The initial application must not claim those events from invented thresholds.

## Cooperative loop

The flight application repeatedly polls bounded subsystem APIs, collects detector and health inputs, steps the state machine, builds telemetry, services the Pi/radio outputs, and services the watchdog. Subsystem modules must not hide their own infinite loops.
