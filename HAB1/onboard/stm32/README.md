# STM32 flight controller

The STM32 is the primary flight controller. Its firmware is written in C and runs as an explicit set of FreeRTOS tasks. One state-management task owns the mission state machine.

```text
apps/flight/       final integrated flight firmware
apps/bringup/      standalone subsystem applications
modules/           reusable drivers and flight services
platform/          board, startup, linker, CMSIS, and FreeRTOS boundary
tests/             STM32-specific host and target test guidance
```

The exact STM32 family/board, FreeRTOS version/integration route, and CMSIS, LL, or HAL choice are still `TBD`. Keep those details below `platform/` so subsystem APIs do not depend on the vendor layer.
