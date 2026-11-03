# Reusable C SD logger core

Implemented in C11 with fixed-size module storage, no explicit heap allocation, board pins,
HAL calls, FreeRTOS dependency, or private tasks. Uses the canonical
`hab_telemetry_record_t`; input values are not sensor samples acquired here.
The existing Arduino SD bench check remains a separate hardware test.

## Storage integration

Provide the six callbacks in `sd_log_storage_t`, then call:

```c
static sd_logger_t logger;
/* storage is a board/filesystem-specific sd_log_storage_t. */
if (sd_logger_init(&logger, &storage) == SD_LOG_OK &&
    sd_logger_start(&logger) == SD_LOG_OK) {
    /* Owning task: drain a bounded number of queued copies each cycle. */
    sd_logger_append(&logger, &record);
    /* Also call on idle cycles, with bounded queue waits. */
    sd_logger_poll(&logger);
    /* On controlled shutdown: */
    sd_logger_stop(&logger);
}
```

Check every return value in actual integration code. `append` confirms a
complete write and may sync if due; use `has_synced_record`, `synced_records`,
and `last_synced_sequence` to distinguish written from successfully synced data.
Poll at a suitably short interval (proposed 100 ms) even when the queue is empty.
Appending continuously also checks sync deadlines, so arrivals cannot starve it.

The implemented `sd_fatfs.c` adapter maps these operations to FatFs. The
`drivers/sd_card/sd_spi.c` disk driver and `sd_spi_hal.c` transport provide SPI
card access and completion/status checks with bounded polling. FatFs R0.16 is
pinned under `third_party/fatfs` with upstream copyright notices.
Only the owning task accesses the file. A sync callback that merely empties a
RAM buffer does not satisfy the contract.

`apps/flight/sd_logging.c` implements a static 32-record queue, zero-wait copied
submission, rejection counters, high-water mark, and protected status snapshots.
`flight_tasks.c` creates the owner task when `HAB_ENABLE_SD_LOGGING=1`; default
is disabled until board configuration is supplied. See
[integration instructions](../../../../docs/sd-logging-integration.md).
No task other than the owner may call the core concurrently. The core itself
never starts tasks or waits on queues.

## Files and errors

Start exclusively creates the first unused `LOG00001.CSV` through
`LOG99999.CSV`, writes a header, and syncs before becoming ready. It never
formats or deletes media. Scanning has a finite limit but may be slow with many
existing files; measure worst-case startup time before flight integration.

CSV version 1 has the columns in [sd-logging.md](../../../../docs/sd-logging.md).
Invalid measurements and GPS metadata are empty. Unknown packet versions,
unknown validity bits, and valid-marked NaN/infinity are rejected before writing.
Finite measurement range validation remains the producer's responsibility.
Nine significant digits preserve float32 values; the C runtime must provide
float formatting (`-u _printf_float` may be necessary with newlib-nano). The
runtime must use a period as its decimal point; incompatible locale is rejected
without changing global locale. Do not change locale concurrently.

The module targets sync at 1000 ms based on elapsed time since successful sync
or the oldest pending record's timestamp. The board clock and record timestamps
must share a timebase and chronological order. Wrap-safe unsigned subtraction
assumes observation intervals and record age stay below half the uint32 range
(about 24.8 days). Counters wrap modulo 2^32.

A short write or storage error latches a fault and stops further appends. A
failed sync does not advance the successful boundary or clear dirty status.
Status tracks maximum record age at sync and counts completed syncs that missed
the five-second budget, so a later successful sync does not hide lateness.
Call `stop` to release an opened handle even after failure; then inspect/report
the original error. After a fault and handle release, recovery requires explicit
reinitialization; retries and card reinsertion policy belong to the application.
Do not reinitialize an object that still owns an open handle. A normal stop/start
creates a new session and clears per-session status/counters.

The one-second software target is not proof of five-second power-cut durability.
Queue age, scheduler delay, callback duration, and card-internal behavior still
require measurement and physical power-cut tests.

## Verification

From the repository root with a host GCC C compiler:

```powershell
New-Item -ItemType Directory -Force work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic HAB1/onboard/stm32/modules/sd_logger/sd_logger.c HAB1/onboard/stm32/tests/sd_logger_test.c -o work/sd_logger_test.exe
./work/sd_logger_test.exe
```

Tests use an in-memory storage adapter with injected failures. They cover exact
CSV output, missing fields, schema/non-finite rejection, new sessions, filename
collisions/exhaustion, short writes, full media, failed mount/sync/close, idle
sync deadlines, continuous arrivals, queued-record age, and timer wrap.
Run all four suites using `tests/run_sd_tests.ps1` from the repository root.
They additionally exercise actual FatFs over RAM media, simulated SPI cards,
and a FreeRTOS queue shim. These do not simulate actual scheduler concurrency.
