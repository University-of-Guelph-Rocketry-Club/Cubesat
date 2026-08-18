# STM32 subsystem bring-up applications

Each directory is a standalone student workspace with its own `main.c`. A bring-up application should initialize one subsystem, exercise its public API, print or expose results, and demonstrate failure behavior.

Reusable implementation belongs under `../../modules/`; the bring-up application is only a small entry point and test harness.

Current workspaces:

- `barometer/`
- `gps/`
- `temperature/`
- `power/`
- `telemetry/`
- `pi_link/`
