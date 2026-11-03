# URC-43: STM32 SD logging interface proposal

Status: proposed integration contract on `codex/urc-43-sd-logging`.
The portable C logger core is now implemented in
[`sd_logger`](../onboard/stm32/modules/sd_logger/README.md). This document retains
the proposed integration contract. The queue, FatFs adapter, SPI driver, and
optional task integration are now implemented. See
[integration/testing](sd-logging-integration.md) for board/producer dependencies
and validation limits.

## Scope and existing decisions

URC-43 requests flight-data storage on an SD card connected to a dedicated SPI
bus, a new file on each power-up, and at most five seconds of data loss after a
power cut. The NUCLEO-F411RE is a development target, not the selected flight PCB.
The available bench card has a nominal capacity of 1 GB.

The current [architecture](architecture.md) assigns primary camera/data logging
to the Pi Zero. The [SD wiring template](../hardware/wiring/sd-card.md) describes
STM32 SD storage as optional. This proposal develops URC-43 as an independent
STM32 logger alongside that design; enabling it in the flight build remains a
team integration decision. It does not replace the Pi logger.

Use the existing
[`hab_telemetry_record_t`](../shared/telemetry/telemetry_record.h) as the initial
input contract. Preserve all its fields, units, version, sequence number,
mission timestamp, validity mask, and status mask. The logger does not poll
sensors, calculate derived measurements, or assign acquisition timestamps.
It must reject unsupported record versions visibly.

The team has not specified whether "all flight data" means combined records or
every sample from every sensor. The current record cannot represent all possible
data: for example, acceleration, magnetometer measurements, raw GPS sentences,
and camera frames are absent. Do not claim full mission-data coverage until the
team agrees the field list and rates. Extend shared schemas through a reviewed,
versioned change if required; do not silently repurpose existing fields.

## Proposed data flow and ownership

```text
Sensor/GPS snapshots
        |
Flight application's record assembly (proposed integration point)
        +--> independent SD queue --> logging task --> filesystem --> SPI SD
        +--> independent Pi forwarding path
        +--> independent radio path (may run at a lower rate)
```

The flight application assembles complete immutable records and fans copies out
to each consumer. A single FreeRTOS queue must not be shared among the logger,
Pi, and radio as competing consumers: each needs its own copy. Record assembly
and this fan-out are not implemented in the current scaffold.

The mission configuration lists logging at 1000 ms and telemetry at 5000 ms,
but is not yet parsed by firmware. Use 1 Hz combined records as the provisional
bench workload. Do not feed the logger only after radio transmission or wait for
a radio acknowledgment. Revisit rate and record assembly when sensor owners
define their actual output rates.

Application code creates the logging task and a statically allocated bounded
queue. Submission copies the record before returning; the caller can reuse its
buffer immediately. Submission is task-context only, with zero queue wait and
no filesystem or SPI operations. Interrupts notify acquisition tasks instead.

The logging task exclusively owns the file, filesystem operations, and SD
driver calls. SPI instance, pins, clocks, and card power belong to the board
configuration/driver, separate from record serialization. Flight integration
must reserve an SPI peripheral for SD rather than merely another chip-select
on the sensor/radio bus.

Suggested module locations for implementation:

- `onboard/stm32/modules/sd_logger/`: serialization and file lifecycle.
- `onboard/stm32/modules/drivers/sd_card/`: SPI card/block-device adapter.
- `onboard/stm32/apps/bringup/sd_card/`: standalone bench application.
- `onboard/stm32/apps/flight/`: queue, task, and record fan-out integration.
- `onboard/stm32/platform/board/`: target-specific configuration.

## Proposed API semantics

These names describe the intended integration boundary. The core implements
start/append/sync/stop/poll and owner-task status snapshots; submission and
concurrency-safe health snapshots remain application responsibilities. See the
module header for the concrete API.

| Operation | Caller | Contract |
| --- | --- | --- |
| `sd_log_submit(record)` | Flight record producer | Copy into the application-owned queue without waiting. Return accepted, queue-full, unavailable, or invalid-record. Accepted means buffered, not durable. |
| `sd_logger_start()` | Logging task | Mount storage, exclusively create a new session file, write/sync its header. Return a visible error on failure. |
| `sd_logger_append(record)` | Logging task | Serialize a queued record with bounded driver timeouts and check both result and bytes written. Does not imply durable storage. |
| `sd_logger_sync()` | Logging task | Flush pending application and filesystem data, including the block driver's completion/sync path. Report success only after completion. |
| `sd_log_get_status()` | Health task | Obtain a concurrency-safe status snapshot without touching the card. |

Queue-full policy: reject the newest submission, preserve existing queue order,
increment a dropped-record counter, and return an error. Never overwrite queued
records silently or block sensing/telemetry indefinitely. Any dropped record is
a logging requirement failure to report, not an acceptable normal operating mode.
Queue capacity must be sized from measured arrival rate and maximum service
stall, with margin; increasing capacity does not fix a missed durability deadline.

Status should expose storage state (starting, ready, fault, full), last error,
accepted/written/dropped counts, queue high-water mark, and the last successfully
synced record sequence/time with a separate "has synced record" flag. Compare
32-bit mission times using wrap-safe elapsed arithmetic. Do not add an SD health
bit to the shared telemetry schema without a reviewed schema change.

On initialization, full-card, write, or sync failure, expose a fault and continue
other flight functions. Do not silently format, delete old sessions, retry
forever, or report queued data as saved. Recovery/reinsert policy remains to be
specified; a new mount/session must never overwrite an existing file.

## File contract proposal

Use CSV initially for straightforward inspection and host-side tests. Serialize
fields explicitly; never write the compiler's struct memory as the file format.
Each file begins with this column header, followed by one complete record per
line in the same order:

```csv
log_format_version,packet_version,sequence_number,mission_time_ms,latitude_deg,longitude_deg,gps_altitude_m,barometric_altitude_m,pressure_pa,temperature_c,humidity_percent,vertical_speed_mps,battery_voltage_v,gps_fix_quality,gps_satellite_count,valid_fields,system_status
```

Proposed log-format version is 1, separate from the shared packet version.
Use decimal integers, numeric bitmasks, a decimal point independent of locale,
and enough float digits to preserve float32 values (nine significant digits).
Invalid measurement fields are empty; masks remain authoritative. Do not turn
missing measurements into valid zeros. Valid-marked non-finite values are an
invalid-record error. The parser must detect/report an incomplete final row
after a power cut rather than invent missing fields.

Use names such as `LOG00001.CSV`, selected by probing for unused names and
exclusive create-new semantics. Never append to a previous boot's file. Do not
depend on GPS time, an RTC, or a repeatedly rewritten boot-counter file. If the
filename space is exhausted, report failure rather than overwrite a session.
Bounded startup scanning and its worst-case latency need bench verification.

For an illustrative 300 bytes per record at 1 Hz, logging uses about 1.08 MB/hour
or 25.9 MB/day. A nominal 1 GB card would hold roughly 38 days before filesystem
overhead and reserve space. This is an estimate, not a guaranteed capacity:
measure serialized size and actual free space. Every-sample logging can be much
larger; at 100 Hz the same record size uses about 108 MB/hour.

## Five-second durability requirement

Measure data age from the flight record's acquisition/assembly timestamp, not
merely from when the logger receives it. Queue delay, scheduling delay, buffered
writes, filesystem metadata, and card completion all count toward the budget.

Propose a 1000 ms sync target to leave margin. This timer must also run when the
queue is idle and must not starve under continuous arrivals. Track pending data
and the last successful durable boundary; do not reset the deadline after a
failed sync. The final timing budget requires measured bounds on the actual
board/card. A sync call every five seconds alone does not demonstrate compliance.

Successful software sync cannot establish immunity to every card-internal
power-failure mode. Repeated abrupt power-cut tests on the intended hardware
must check the readable file, missing timestamps, and earlier sessions. If the
strict requirement cannot be met with the chosen card/power design, report that
result for a hardware/storage decision rather than claim compliance.

## Decisions and later acceptance tests

Pending team decisions:

1. Confirm independent STM32 SD logging is enabled alongside Pi logging.
2. Define "all flight data": fields, every-sample versus snapshot recording,
   rates, maximum data age, and intended flight duration.
3. Confirm CSV or another reviewed file format and downstream reader ownership.
4. Select flight board, dedicated SPI instance/pins, and adapter supply circuit.
5. Agree runtime recovery behavior and how logging faults affect mission readiness.

Later implementation must demonstrate: unique files across repeated boots;
round-trip field/validity preservation; observable version/queue/storage errors;
no stalled sensing or telemetry during SD faults; measured queue/service/sync
timing at the agreed workload; and power-cut loss of no more than five seconds
without loss of earlier sessions. Bench results on the Nucleo do not substitute
for validation on the final flight controller.
