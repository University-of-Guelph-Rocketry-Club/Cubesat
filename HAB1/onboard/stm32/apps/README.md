# STM32 applications

An application owns `main.c`, creates its FreeRTOS tasks, and selects which reusable modules to link.

- `flight/` is the final integrated mission application.
- `bringup/` contains focused applications that exercise one subsystem at a time.

Do not copy driver implementations from bring-up applications into the flight application. Both application types should link the same module once hardware-specific code exists.
