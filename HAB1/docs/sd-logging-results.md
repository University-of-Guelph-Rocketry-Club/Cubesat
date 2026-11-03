# URC-43 local test results — 2026-11-02

Review branch: `codex/urc-43-sd-logging`. These are bench results, not merge or flight-acceptance approval.

Latest bench status: missing-card startup, reinsertion, and two additional
physical power cuts passed. All 14 recovered CSV files parsed correctly; the
two new interrupted sessions preserved every captured sample and all prior
file checksums. The initial trial's delayed recovery remains part of the
history below. Final-board, real-workload and scheduler validation are separate.

## Software checks

| Check | Result | Boundary |
| --- | --- | --- |
| C core tests | PASS | CSV/masks/float32, unique sessions, invalid values, short writes, errors, wrap, deadlines |
| FatFs integration | PASS | Actual R0.16 filesystem on isolated FAT16 RAM media, exclusive names, full media, disk failures |
| SPI card simulation | PASS | SDSC/SDHC initialization/addressing/read/write, rejected writes, missing response, busy/transport timeouts |
| Queue integration | PASS | Copied zero-wait submissions, overflow, no producer I/O, idle sync, fault cleanup, protected snapshots |
| ARM compile check | PASS | Enabled flight task, queue service and STM32F4 board adapter with FreeRTOS V11.2.0 headers |
| C bench firmware | PASS build | GNU Arm GCC 14.2.1; C/HAL/FatFs; 32,092 text + 468 data + 4,420 BSS bytes |

Run `onboard/stm32/tests/run_sd_tests.ps1` from the repo root. For target API
checking, run `tests/check_sd_arm.ps1 -KernelPath <FreeRTOS-Kernel-V11.2.0>`.
The test-only RTOS configuration and queue shim do not validate real scheduler
concurrency or the final flight build. Vendor HAL has unused-parameter warnings;
the minimal newlib-nano link reports unused POSIX syscall stubs. These are not
SD/FatFs I/O: that path uses explicit storage callbacks.

## Normal hardware runs

Hardware: NUCLEO-F411RE; user's nominal 1 GB microSD; regulated 5V-input SPI
adapter. SPI1 PA5/6/7 with PB6 CS (CN10 pins 11/13/15/17), initialized at 250kHz,
run at 1MHz. ST-Link serial COM7, 115200 baud. No real sensors were attached.

The all-C bench firmware created LOG00001.CSV, LOG00002.CSV, and LOG00003.CSV
in separate sessions. It wrote 12 explicitly synthetic 1Hz records in each,
periodically synced, closed/reopened, and exported the file contents. Independent
host parsing confirmed the 17-column schema, exact temperature/pressure values,
sequence, validity mask, and blank invalid measurements in two captured sessions.

Maximum observed acquisition-to-sync completion age: **1011 ms** for records with
captured sync acknowledgments. The final row was flushed at controlled close;
this is not a measurement of a universal worst-case latency or power-loss bound.

Earlier-session checks:

| File | Bytes | FNV-1a checksum | Records |
| --- | --- | --- | --- |
| LOG00001.CSV | 722 | 9f41719a | 12 |
| LOG00002.CSV | 722 | 8c07e4fa | 12 |
| LOG00003.CSV | 722 | 9aebbbb2 | 12 |

Audits across resets preserved earlier closed-session sizes/checksums. Repeated
boot filenames were unique. The checksum is an inspection aid, not a cryptographic
integrity mechanism or on-disk CRC format.

Captures under ignored `work/` include `c_logger_bench_1.txt`,
`c_logger_bench_2.txt`, and `c_logger_bench_3.txt`. Independent validation command:

```powershell
python HAB1/onboard/stm32/tests/verify_bench.py work/c_logger_bench_2.txt work/c_logger_bench_3.txt
```

## Initial physical power-cut trial — delayed recovery history

The operator confirmed unplugging/reconnecting Nucleo USB during continuous
synthetic logging to LOG00004.CSV. The last captured sample was sequence 9 at
49334 ms. The last successful captured sync covered sequence 8 at 48428 ms.
The firmware reported:

```text
FAIL sync result=3 fatfs=1 cmd=24 response=0x00
```

The logger exposed a fault instead of falsely advancing its sync boundary.
After reconnect the C driver could not initialize the card. A fresh firmware
restart and operator-confirmed 10-second power-off/reseat did not restore it.
The original Arduino test subsequently initialized once but failed file creation.
A separate read-only Arduino inspection later failed initialization as well.
Reported CMD0 responses included 0xff and 0x00. These observations do not isolate
the cause among card state/failure, module power, contact/wiring, or firmware
recovery behavior. The bounded CMD0 recovery retry was increased to two seconds;
software tests still passed, but that change did not restore this hardware run.

At this stage, the card's post-cut contents and loss count had not been
recovered/verified; the later update below records successful recovery. Do not
format/repair the card merely to obtain a passing result: inspect/save it with
a separate reader first. No format, filesystem repair, or deletion was performed.
The Nucleo is left running the read-only recovery inspection firmware.

Captures: `work/c_logger_powercut_before.txt`, `c_logger_powercut_after.txt`,
`c_logger_powercut_recovery.txt`, `c_logger_reseated_reset.txt`,
`arduino_after_powercut.txt`, `c_logger_recovery_retry.txt`,
and `sd_readonly_recovery.txt`.

## Remaining acceptance work

### Two additional physical power cuts and complete read-only recovery

After the operator reinserted the card while unpowered, the C firmware created
LOG00010.CSV and passed a 12-record write/sync/reopen test (maximum observed
sync age 944 ms). The physical missing-card/reinsertion sequence was successful.

The operator then performed two USB power cuts, leaving the card inserted and
waiting five seconds before reconnecting. A serial monitor spanning each cut
captured the host disconnect and automatic MCU/card restart. No firmware reflash
was required to recover either trial.

| Trial | Interrupted file | Captured/saved records | Saved tail | Startup after reconnect |
| --- | --- | --- | --- | --- |
| 2 | LOG00011.CSV | 40 / 40 | seq 39 at 80252 ms | Automatic; new LOG00012.CSV |
| 3 | LOG00013.CSV | 32 / 32 | seq 31 at 59830 ms | Automatic; new LOG00014.CSV |

Each restart's 12-record C logging run passed independent CSV verification;
maximum observed sync ages were 834 ms and 746 ms respectively. Read-only
Arduino export then recovered all 14 CSV files. Independent parsing verified
every synthetic value, sequence and complete row; all captured pre-cut sample
timestamps matched the recovered interrupted rows exactly. Ten prior file
checksums were unchanged for trial 2; twelve were unchanged for trial 3.

Interrupted files: LOG00011.CSV = 1823 bytes / FNV-1a 6834ec79;
LOG00013.CSV = 1511 bytes / FNV-1a ec222ccb. Neither has a partial final row.
These two trials lost no **captured** samples; the exact electrical cutoff
instant and any UART samples not delivered before disconnect were not measured.
They are positive bench evidence, not a universal power-loss guarantee.

Captures: `work/powercut_trial2.txt`, `work/powercut_trial3.txt`,
`work/hardware_cuts_recovered.txt`; validation summary:
`work/hardware_cuts_validation.txt`. Reboot-only slices for `verify_bench.py`:
`work/powercut_trial2_reboot.txt` and `work/powercut_trial3_reboot.txt`.
The Nucleo is left on read-only recovery inspection firmware after exporting.

### Physical missing-card startup check

With USB disconnected, the operator removed the card and reconnected USB with
the slot empty. The C bench firmware reported `FAIL start result=3 fatfs=3
cmd=0 response=0x00` over ST-Link serial. This confirms an unavailable-card
startup is reported as a storage fault; the response does not uniquely identify
card absence. Initialization failed before file creation. Capture:
`work/hardware_missing_card.txt`. This bench check does not independently prove
scheduler isolation in the integrated flight application.

### Readability/documentation regression check

Added comments on module ownership, callbacks, protocol stages, invalid fields,
sync boundaries and failure cleanup; expanded control-flow formatting and terse
variable names; replaced raw SD command numbers with named constants. Vendor
FatFs source was left untouched. `HAB1/WHAT_WE_ADDED.md` inventories every added
file and existing modification, distinguishing production code and test tools.

All four host suites and the enabled ARM compile checks passed after refactoring.
The complete C firmware rebuilt with the same 32,092-byte text / 468-byte data /
4,420-byte BSS totals. It was flashed and passed a new 12-record write/sync/reopen
run to LOG00008.CSV; previous session checksums remained unchanged. Independent
CSV/timestamp validation also passed. Capture: `work/readability_bench.txt`.
This check does not add a new physical power-cut acceptance result.

### C write/read retest after computer restart

After the operator restarted the computer, the current C firmware was rebuilt
and flashed. All four host suites and enabled ARM compile checks passed again.
The card initialized and the C logger created LOG00005.CSV, wrote 12 synthetic
records, periodically synced, reopened, and exported them. Independent parsing
verified every value. Audits confirmed LOG00001/2/3 retained their original
checksums, and LOG00004 retained 613 bytes, checksum b1ae6ac4, and nine rows.
This restores actual C-path write/read operation; it does not establish the
cause of the prior intermittent failures. Capture: `work/submission_bench.txt`.
The Nucleo now runs the C bench firmware rather than the read-only inspector.
Two additional MCU resets created LOG00006.CSV and LOG00007.CSV. Each exported
12 exact records; independent host parsing passed, earlier-session checksums
were unchanged, and maximum observed sync age was 1011 ms. Captures:
`work/submission_reset_1.txt` and `work/submission_reset_2.txt`.

### Recovery update after operator restart

The read-only Arduino inspection succeeded after a later operator restart.
All four CSV files were exported and independently parsed. LOG00001/2/3 each
contain all 12 exact synthetic records and match their pre-cut byte counts and
FNV-1a checksums above. LOG00004.CSV is 613 bytes with nine complete, exact
records (sequence 0 through 8), ending at mission time 48329 ms. Its final row
is newline terminated. The last captured generated sample was sequence 9 at
49334 ms, so one captured sample is absent: a 1005 ms gap between those sample
timestamps. The actual electrical cutoff time was not measured. This trial is
consistent with losing only the pending tail; it does not prove a universal
five-second bound or resolve the intermittent startup failure.

No corruption was found in the recovered CSV contents or earlier-session
checksums. This is not a full filesystem consistency scan or proof that all
card sectors are healthy. Capture: `work/sd_after_restart.txt`. The Nucleo
remains on read-only inspection firmware; writing was not retested in this update.

- Characterize the initial intermittent startup failures with power/signal
  measurements or substitute hardware if they recur; later runs recovered normally.
- Extend power-cut tests to the final controller and agreed maximum workload,
  measuring actual cutoff timing when assessing a strict loss bound.
- Test live removal/full-media behavior and the agreed maximum
  record rate; host fault injection alone is insufficient.
- Connect the team's actual record assembler and selected board/RTOS build;
  measure stack, heap, queue stalls and task deadlines with all subsystems active.
- Confirm the shared record covers all required flight data and resolve the
  optional STM32 logger versus Pi-first architecture decision with the team.

The implementation is ready for local code review and further bench validation;
it is not a completed flight-acceptance result.
