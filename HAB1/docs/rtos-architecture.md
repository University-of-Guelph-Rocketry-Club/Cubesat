# FreeRTOS architecture

FreeRTOS is the STM32 execution environment for HAB-001. The architecture keeps ownership explicit and communication bounded.

## Initial tasks

| Task | Responsibility | Expected activation |
| --- | --- | --- |
| Sensor acquisition | Poll environmental and power modules; publish validated snapshots | Periodic |
| GPS | Receive and parse GPS data; publish fix snapshots | Notification and/or periodic |
| State management | Sole owner of the mission state machine; consume detector and health inputs | Event-driven with bounded periodic check |
| Telemetry | Build versioned records and service the selected radio | Periodic and queued |
| Pi link | Forward bounded versioned frames without blocking critical work | Queue-driven |
| System health | Check task heartbeats, stale data, and faults; gate watchdog servicing | Periodic, high assurance |

Priorities, periods, stack depths, and queue lengths are intentionally `TBD` until execution time and memory use are measured on the selected STM32.

## Ownership and communication

- The state-management task is the only task allowed to mutate the mission state machine.
- Sensor and GPS tasks publish immutable snapshots rather than exposing mutable globals.
- Use queues for records and snapshots, direct task notifications for lightweight wakeups, and event groups for readiness or heartbeat bits.
- Use mutexes only for genuinely shared resources and hold them for bounded durations.
- Define queue-full and timeout behavior explicitly; dropping non-critical Pi data must not stall sensing or primary telemetry.
- Interrupt handlers perform minimal capture/acknowledgement and wake a task for further processing.

## Allocation and watchdog policy

Flight tasks should use `xTaskCreateStatic` and statically allocated RTOS objects where practical. The health task should service the hardware watchdog only after required task heartbeats and critical health conditions have been checked. A single healthy task must not be able to mask a deadlocked flight system.

## Current status

Task entry points and static task creation are scaffolds. Queues, event groups, detector algorithms, final priorities, stack measurements, and the target `FreeRTOSConfig.h` are not yet implemented.
