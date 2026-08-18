# Getting started

This guide takes a new student from the repository to their first subsystem exercise.

## 1. Clone the repository

```bash
git clone https://github.com/University-of-Guelph-Rocketry-Club/Cubesat.git
cd Cubesat/HAB1
```

Read the [architecture](architecture.md) overview before changing integrated flight code.

## 2. Choose one example

Start with one README under [`onboard/stm32/apps/bringup/`](../onboard/stm32/apps/bringup/), such as:

- `barometer/` for pressure and altitude;
- `gps/` for position and fix quality;
- `temperature/` for a simple environmental measurement;
- `power/` for battery monitoring;
- `telemetry/` for the shared record shape; or
- `pi_link/` for STM32-to-Pi communication.

Pi Zero students can instead start under [`onboard/pi-zero/apps/bringup/`](../onboard/pi-zero/apps/bringup/) with camera, STM32-link, or storage exercises.

## 3. Check hardware first

The STM32 architecture and Pi Zero role are selected, but exact models and toolchains are not. Before a hardware example can be run:

1. record the selected STM32 family/board or Pi hardware and sensor interface;
2. record the selected part and interface in `config/sensors/`;
3. record approved pins and voltage levels in `hardware/`;
4. select the toolchain and library versions; and
5. add exact build, flash, run, and observation commands to the workspace README.

Until those decisions are made, STM32 bring-up source is a hardware-neutral FreeRTOS task skeleton. It does not generate fake measurements.

## 4. Make a small change

Use a branch such as `feature/barometer-driver` or `docs/gps-testing`. Keep changes focused and explain how they were tested in the pull request. See [CONTRIBUTING.md](../CONTRIBUTING.md).

## 5. Before flight use

Do not treat a bring-up application or the integrated skeleton as mission-ready. Follow the [testing progression](testing.md), record hardware results, and have flight-impacting changes reviewed by the appropriate team lead.
