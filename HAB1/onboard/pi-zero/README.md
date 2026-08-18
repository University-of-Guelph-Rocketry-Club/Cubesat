# Raspberry Pi Zero data logger

The Pi Zero is an onboard Linux companion computer connected to a Raspberry Pi Camera. It receives STM32 records, preserves raw data, captures camera data, and manages storage.

The Pi is not required for the STM32 to boot, sense, determine mission state, or transmit primary telemetry. Pi failure must be visible but non-critical to the STM32 flight loop.

The operating system, Pi model, camera model, application language, and STM32 link remain `TBD`. Raspberry Pi OS with a small userspace service is the expected starting point; the Pi does not run the STM32 FreeRTOS firmware.
