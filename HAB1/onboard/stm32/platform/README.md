# STM32 platform boundary

Board- and vendor-specific code belongs here:

- `board/` exposes the small interface used by applications;
- `startup/` will contain the selected STM32 startup source and vector table;
- `linker/` will contain the selected memory layout;
- `cmsis/` will document or reference the approved vendor/CMSIS files;
- `freertos/` will document the approved kernel version, port, and configuration.

The exact STM32 family, board, clock tree, FreeRTOS version/port, and CMSIS/LL/HAL choice remain `TBD`.
