# STM32 reusable modules

Modules contain code shared by standalone bring-up applications and the integrated flight application.

- `drivers/` owns device and transport boundaries.
- `state_machine/` owns mission states and transition sequencing.
- `system_health/` owns consolidated subsystem status policy.

A module should expose a small C API, explicit units, bounded execution, and visible error/status results. Avoid dynamic allocation and hidden infinite loops.
