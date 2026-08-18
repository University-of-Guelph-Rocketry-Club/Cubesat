# Onboard software

All software carried by the balloon lives here.

- `stm32/` contains the bare-metal C firmware for the primary flight controller.
- `pi-zero/` contains Linux userspace software for camera capture and data logging.
- `integration/` defines how the two computers are assembled and tested for a mission.

The STM32 remains responsible for flight sensing, state handling, health monitoring, and telemetry even if the Pi Zero is unavailable.
