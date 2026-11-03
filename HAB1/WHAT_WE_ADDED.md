# URC-43: What we added

This branch adds an independent **STM32 SD-card logger** for HAB flight-data
records. It creates a new CSV file per startup/session, targets a sync every
second, and exposes storage failures instead of silently losing records.
It runs on the flight controller; the NUCLEO-F411RE is our bench development
board. The Pi Zero logger remains a separate part of the existing architecture.

Repository: `C:\coding\Cubesat`

Branch: `codex/urc-43-sd-logging`

Delivery: feature branch for review. Publication does not imply merge or completed flight acceptance.

## How the pieces connect

```text
Real flight-record assembler (team integration point)
    |
    | flight_log_record(&record): copy, do not wait for the SD card
    v
Static FreeRTOS queue -> Logging task -> Portable C logger
                                           |
                                           v
                                      FatFs adapter
                                           |
                                           v
                                  SPI card/block driver
                                           |
                                           v
                                STM32 HAL + board SPI/pins
                                           |
                                           v
                                      microSD card
```

The input is the existing `shared/telemetry/telemetry_record.h` record. We did
not change that schema or invent real sensor measurements. The bench program
generates explicitly synthetic records for testing.

**Queued**, **written**, and **synced** mean different things:

- A successful producer submission means the queue owns a copy of the record.
- A successful write means the filesystem accepted the serialized row.
- A successful sync is the software's confirmed saved boundary, after filesystem
  flushing and the card driver's completion checks. It is not a universal
  guarantee against every SD-card power-failure mode.

## Production files added

The following paths are relative to `HAB1/onboard/stm32/`.

| File | What it does |
| --- | --- |
| `modules/sd_logger/sd_logger.h` | Public C API, storage callback contract, status fields, fixed buffer sizes, sync/loss-budget constants. |
| `modules/sd_logger/sd_logger.c` | CSV serialization, unused session filenames, startup/header sync, append/poll/sync/stop lifecycle, fault handling, and saved-record boundaries. |
| `modules/sd_logger/sd_fatfs.h` | FatFs adapter state: filesystem, file handle, original FatFs error, and mission clock callback. |
| `modules/sd_logger/sd_fatfs.c` | Maps logger operations to FatFs. Uses exclusive create-new, detects short writes/full media, and releases a failed handle safely. |
| `modules/sd_logger/README.md` | Core ownership, input/output rules, timing assumptions, errors, and test entry points. |
| `modules/drivers/sd_card/sd_spi.h` | Portable SPI transport interface and card state; the board supplies transfers, CS, speed, clock and delay callbacks. |
| `modules/drivers/sd_card/sd_spi.c` | SD startup commands, SDSC byte/SDHC block addressing, sector reads/writes, busy/status checks, and FatFs drive-0 disk functions. |
| `modules/drivers/sd_card/sd_spi_hal.h` | STM32F4 transport configuration: SPI handle, CS port/pin and clock prescalers. |
| `modules/drivers/sd_card/sd_spi_hal.c` | Bounded HAL full-duplex transfers, active-low CS control, and initialization/run speed selection. |
| `platform/board/board_sd.h` | Board setup API called before logging tasks start. |
| `platform/board/board_sd.c` | Connects the selected board's SPI/pins to the SPI driver and FatFs adapter; uses the same mission timebase as flight records. |
| `apps/flight/sd_logging.h` | Producer/service/status API and queue statistics. |
| `apps/flight/sd_logging.c` | Static 32-record queue, zero-wait copied submissions, overflow/invalid counts, owner-task servicing, and protected health snapshots. |

## Existing production files modified

| File | Modification |
| --- | --- |
| `onboard/stm32/apps/flight/flight_tasks.c` | Adds the optional static logging task, configures storage before task startup, exposes `flight_log_record`, and makes a logging-status snapshot available in the health-task scaffold. |
| `onboard/stm32/apps/flight/flight_tasks.h` | Declares `flight_log_record` and documents that acceptance means queued, not durable. |
| `onboard/stm32/modules/README.md` | Lists the new logger module and its separation from drivers/application tasks. |
| `docs/telemetry.md` | Links the existing shared record to the SD logging proposal. |
| `hardware/wiring/sd-card.md` | Links URC-43's independent logger proposal and explicitly leaves final flight-board pins/power unselected. |
| Repository-root `.gitignore` (new) | Excludes `/work/` so compiler outputs, downloaded test dependencies, and serial captures stay out of source control. |

The flight logging feature defaults to **disabled**. The board must initialize
its dedicated SPI/pins and call `board_sd_configure`, then the build can define
`HAB_ENABLE_SD_LOGGING=1`. The real record assembler must call
`flight_log_record` at the agreed logging rate. We did not fill the original
sensor/GPS TODOs with fake data or modify the team's watchdog policy.

## Bench tools added

Paths below are relative to `onboard/stm32/apps/bringup/sd_card/`.

| File | What it does |
| --- | --- |
| `README.md` | Wiring, original Arduino test instructions, and links to the C logger and results. |
| `.gitignore` | Excludes the original Arduino test/recovery build directory. |
| `sd_card_test/sd_card_test.ino` | Initial Arduino comparison test: initialize, create a uniquely named test file, write, close/reopen, and compare contents. Reports card/volume failures. |
| `sd_recovery/sd_recovery.ino` | Read-only Arduino recovery aid: lists files and exports existing LOG CSV contents. Does not format or repair the card. |
| `c_logger/main.c` | All-C Nucleo bench application using the production core/FatFs/SPI layers. Writes 12 synthetic records, checks saved files and prior-session checksums, exports CSV, and accepts reset/continuous-test commands. |
| `c_logger/build.ps1` | Builds C/HAL/FatFs firmware using installed GNU Arm GCC and the repository's existing STM32F411 HAL/startup/linker sources. Enables float formatting. |
| `c_logger/.gitignore` | Excludes generated C firmware, object files and maps. |

The C bench uses PA5/PA6/PA7 for SPI1 and PB6 for CS, at 250kHz startup and
1MHz operation. Those pins are a **bench configuration**, not a final custom-PCB
assignment. The Arduino sketches are diagnostic tools, not production flight
dependencies. Synthetic bench records must never be presented as flight data.

## Tests added

Paths below are relative to `onboard/stm32/tests/`.

| File | What it checks |
| --- | --- |
| `sd_logger_test.c` | CSV values/masks/float precision, schema rejection, unique files, filename exhaustion, short writes, error cleanup, sync timing, late-sync reporting and clock wrap. |
| `sd_fatfs_test.c` | Real FatFs over an isolated FAT16 RAM disk: file contents, create-new behavior, disk errors and full media. Never writes a physical disk. |
| `sd_spi_test.c` | Simulated SDSC/SDHC command responses, addressing, block data, rejected writes, absent responses, busy and transport failures. |
| `sd_logging_queue_test.c` | Queue copies, zero-wait submissions, full-queue rejection, absence of producer I/O, idle sync and fault snapshots/cleanup. |
| `rtos_stub/FreeRTOS.h` | Minimal host-only types and queue control state. |
| `rtos_stub/queue.h` | Host-only queue declarations implemented by the queue test. |
| `rtos_stub/task.h` | Host-only critical-section hooks checked by the queue test. |
| `arm_config/FreeRTOSConfig.h` | Compile-check-only RTOS configuration; not a flight configuration. |
| `run_sd_tests.ps1` | Builds and runs all four host test executables with warnings treated as errors. |
| `check_sd_arm.ps1` | Compiles enabled flight-task, queue and board-adapter sources against real Cortex-M4F FreeRTOS headers. |
| `verify_bench.py` | Independently validates exported synthetic CSV values, schema, sync timestamps and earlier-file checksums from serial captures. |

The RTOS shim checks API behavior, not actual preemptive scheduling. The ARM
check verifies target compilation, not a complete integrated flight build.

## Third-party files added

`onboard/stm32/third_party/fatfs/` contains FatFs R0.16:

| File | Purpose/change |
| --- | --- |
| `ff.c` | Upstream filesystem implementation; original source/notices retained. |
| `ff.h` | Upstream public filesystem types/API. |
| `diskio.h` | Upstream block-device API implemented by our SPI driver. |
| `ffconf.h` | Configuration: code page 437 and fixed-date/no-RTC mode. Short filenames, one volume, 512-byte sectors, no exFAT or formatting API. |
| `00readme.txt` | Upstream package information. |
| `README.md` | Source/version, configuration differences and single-owner requirements. |

Vendor sources were not reformatted during the readability pass. Our C source
uses expanded control flow, four-space indentation, named SD commands and
comments explaining ownership, protocol stages and failure behavior.

## Documentation added

| File | Purpose |
| --- | --- |
| `WHAT_WE_ADDED.md` | This file: change inventory and reviewer entry point. |
| `docs/sd-logging.md` | Original interface/data-format proposal and team decisions still pending. |
| `docs/sd-logging-integration.md` | Production build/board/producer wiring, test commands and limitations. |
| `docs/sd-logging-results.md` | Actual measured passes, intermittent failures, power-cut recovery and remaining acceptance work. |
| `docs/urc-43-progress.md` | Assignment checklist and milestone history. |
| `docs/urc-43-handoff.md` | Suggested task update and reviewer handoff; distinguishes review readiness from completed acceptance. |

## How to review and retest

From `C:\coding\Cubesat`:

```powershell
./HAB1/onboard/stm32/tests/run_sd_tests.ps1
./HAB1/onboard/stm32/tests/check_sd_arm.ps1 -KernelPath work/freertos/FreeRTOS-Kernel-11.2.0
./HAB1/onboard/stm32/apps/bringup/sd_card/c_logger/build.ps1
```

The second command needs a local FreeRTOS-Kernel tree. The ARM scripts document
the installed compiler path; adapt it on another machine. The hardware test
requires the actual Nucleo/card/wiring and replaces its bench firmware when
flashed. Full instructions and evidence are in `docs/sd-logging-integration.md`
and `docs/sd-logging-results.md`.

## What is still not accepted

Normal logging, periodic sync, unique reset files and preserved earlier records
were demonstrated. One interrupted run recovered records 0-8 and lacked captured
record 9: a 1005ms sample-timestamp gap. We did not measure the exact electrical
cutoff instant. Startup failures occurred afterward and later cleared following
a restart; their cause is unresolved.

Two additional physical cuts later saved all 40 and 32 captured samples and
recovered automatically. All 14 CSV files and prior-session checksums were
verified. Physical missing-card startup and reinsertion also passed. Final-board
workload/scheduler measurements and additional physical fault coverage remain.
The team must also confirm full-data coverage, record rates,
real producer integration, board pins and readiness policy. The feature is ready
for local review; this document does not label it flight-ready or certify the
five-second power-loss requirement.
