# URC-43 integration and testing

Changes are prepared on review branch `codex/urc-43-sd-logging`.
The STM32 logger is independent of the Pi logger. Team agreement on enabling
this feature and on full-data coverage is still required.

## Code layout

Relative to `onboard/stm32/`:

| Source | Responsibility |
| --- | --- |
| `modules/sd_logger/sd_logger.c` | CSV records, unique sessions, sync policy |
| `modules/sd_logger/sd_fatfs.c` | FatFs mount/create/write/sync/close |
| `modules/drivers/sd_card/sd_spi.c` | Dedicated SPI card, drive-0 disk I/O |
| `modules/drivers/sd_card/sd_spi_hal.c` | STM32F4 HAL transfer and CS |
| `platform/board/board_sd.c` | Board configuration and callback factory |
| `apps/flight/sd_logging.c` | Static queue, copied submission, status snapshots |
| `apps/flight/flight_tasks.c` | Optional logging task and producer entry point |

FatFs R0.16 is vendored with original notices. Configuration: one FAT12/16/32
volume, 512-byte sectors, short filenames, no mkfs/TRIM/exFAT, no filesystem
reentrancy. Only the logger task accesses it. The driver supports SDSC byte and
SDHC block addressing and uses single-sector CMD17/24 transfers. It checks busy
completion and CMD13 programming status, and uses finite polling/device timeouts.
Data CRC is not enabled; validate electrical signal integrity on flight hardware.

## Flight integration

1. Add the production sources above and `third_party/fatfs/ff.c` to the flight
   build, with the corresponding include paths, HAL, startup and runtime.
   Enable float printf support (newlib-nano: `-u _printf_float`). The module
   uses fixed storage but libc float formatting may allocate internally; measure
   stack/heap use rather than relying on static RAM totals.
2. Initialize a dedicated SPI bus in `board_init`: mode 0, 8-bit, MSB first,
   CS output initially high, proper power and stabilization. Call
   `board_sd_configure(spi, cs_port, cs_pin, init_prescaler, run_prescaler)`.
   Choose 100-400kHz initialization and a tested run rate from actual bus clocks.
   Nucleo bench values: HSI/PCLK2 16MHz, SPI1 PA5/6/7, PB6 CS, /64 then /16.
   These are not a pin approval for the team's custom PCB.
3. Define `HAB_ENABLE_SD_LOGGING=1`. The task starts storage then processes one
   record per nominal 10ms cycle. Queue length is 32; static stack is
   `configMINIMAL_STACK_SIZE + 1024` words. Rates, priorities, stack and maximum
   device stalls need measurement on the selected flight board.
4. From the real record assembler, call `flight_log_record(&record)` at the
   agreed logging rate. `pdPASS` means copied/queued, not durable. The actual
   acquisition/fan-out remains a baseline TODO; placeholder sensor records are
   deliberately not submitted as real data. Logging must not depend on the
   slower radio schedule or on successful transmission.
5. Read `hab_sd_status` for accepted/dropped/invalid counts, queue high-water,
   storage fault, last successful sync boundary, maximum record age at sync,
   and missed durability deadlines. Decide mission readiness/watchdog policy
   with the team. No new health bits or invented sensor values are introduced.

Producer submission never performs filesystem I/O and rejects the newest record
if full/unavailable. Queued records are independent copies. Storage failure
stops acceptance and releases the handle; no silent overwrite, formatting,
deletion, or indefinite retry occurs. Recovery requires explicit restart policy.
The logging task owns all core calls; other tasks read protected snapshots.
The clock callback and record mission time must share a monotonic timebase.

The full flight application is still a scaffold: its board implementation,
selected RTOS configuration, acquisition and target build contain unrelated
baseline TODOs. Enabled flight/queue sources were compiled against FreeRTOS
V11.2.0 Cortex-M4F headers using `tests/arm_config` for compile checking only.
This does not select the final mission RTOS configuration or prove scheduler
timing on the flight board. Logging is disabled by default until configured.

## Repeatable host tests

From the repository root with GCC and PowerShell:

```powershell
./HAB1/onboard/stm32/tests/run_sd_tests.ps1
```

Four suites test the core, real FatFs on an isolated FAT16 RAM disk, a simulated
SDSC/SDHC SPI card, and a FreeRTOS queue shim. Coverage includes exact CSV and
float32 values, invalid/schema rejection, name collisions/exhaustion, idle and
loaded sync deadlines, timestamp wrap, late-sync reporting, mount/write/sync/
close failures, full media, SPI timeouts, copied records and queue overflow.
The queue shim tests API behavior, not actual preemptive scheduling. No host
test touches physical media; full-disk testing fills only RAM storage.

## C-only bench firmware

```powershell
./HAB1/onboard/stm32/apps/bringup/sd_card/c_logger/build.ps1
```

The build script uses GNU Arm GCC 14.2.1 from the installed STM32 Arduino package
as a compiler only. Firmware is C/HAL/FatFs, with no Arduino runtime. It reuses
the checked-in STM32F411 HAL/startup/linker sources currently under the barometer
workspace. Adjust the compiler path in the script for another installation.

Copy `c_logger/build/logger.bin` onto the verified `NOD_F411RE` ST-Link drive.
This replaces Nucleo firmware. Open the ST-Link serial port at 115200 baud.
The test creates a new file each start, emits 12 SYNTHETIC records at nominal
1Hz, periodically syncs, closes/reopens, and exports actual file contents. It
audits older sessions using byte count, checksum, row count and last timestamp.
After `READY`, send `R` to reset or `P` to start a continuous synthetic session.
Do not use this synthetic application as flight firmware.

The card needs an existing FAT16/FAT32 filesystem. No format is performed.
FatFs timestamps use a fixed fallback date; CSV mission times are authoritative.
`tests/verify_bench.py capture1.txt capture2.txt` independently checks exported
values, 17 columns, masks, earlier-session hashes, and observed sync ages.
Captures go under ignored `work/`; findings belong in the results document.

## Physical power-cut procedure

Capture serial after `P`, disconnect all board power at an unpredictable point,
wait at least three seconds, and reconnect. Compare the last emitted sample to
the recovered session's last complete record and verify all earlier closed
session checksums. Report a partial final row rather than treating it as complete.
Inspect recovered CSV with a separate reader where available. Repeat at different
write/sync phases and at the agreed maximum data rate.

One cut does not establish a universal five-second guarantee. Include filesystem
readability and card startup recovery in results. If the card will not initialize
after a cut, the test remains unresolved even if earlier syncs succeeded. Host
fault injection does not replace missing/full-card and power-cut hardware tests.
