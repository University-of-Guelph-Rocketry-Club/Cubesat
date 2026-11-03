# URC-43 review handoff

## Proposed task update

Implemented an independent STM32 C SD logger with per-boot/session CSV files,
one-second sync target, FatFs/SPI storage, static FreeRTOS queue/task integration,
and visible storage/overflow/deadline status. The feature is disabled by default
until the selected board supplies the dedicated SPI configuration. The shared
HAB telemetry record is the input; the actual sensor record assembler remains
a team integration dependency.

Four host suites and ARM compilation pass. NUCLEO-F411RE tests verified exact
synthetic CSV contents, periodic sync, unique filenames and preserved earlier
sessions. One physical power-cut trial recovered all earlier files unchanged
and lost one captured sample (1005 ms between last saved/generated sample
timestamps). Intermittent initialization failures occurred after that cut;
a later restart restored access, and a subsequent full C write/read run passed.
The precise electrical cutoff time was not measured. This is ready for review,
not a claim of completed flight acceptance or a guaranteed five-second bound.
Two additional physical cuts later recovered automatically and preserved all
40 and 32 captured samples respectively. All 14 CSV files were exported and
verified, with prior-session checksums unchanged. Physical missing-card startup
and card reinsertion also passed. See the results document for the complete history.

Implementation branch: `codex/urc-43-sd-logging`. The user authorized publishing
this branch and opening a review PR. No merge or task-completion action is implied.

## What reviewers need

- Read [what we added](../WHAT_WE_ADDED.md) for the complete file/change inventory.

- Start with [integration instructions](sd-logging-integration.md).
- Read [measured results](sd-logging-results.md), including unresolved failures.
- Review `onboard/stm32/modules/sd_logger/`, `modules/drivers/sd_card/`,
  `platform/board/board_sd.*`, and the optional flight-task changes.
- Run `onboard/stm32/tests/run_sd_tests.ps1` from the repository root.
- Build the C-only Nucleo bench using
  `onboard/stm32/apps/bringup/sd_card/c_logger/build.ps1`.
- Synthetic bench firmware is for testing only; do not copy it into flight code.

## Before marking the assignment complete

1. Extend the three recorded bench cut trials to the final controller/workload.
   Characterize earlier intermittent startup behavior if it recurs, using
   supply/signal measurements rather than assuming filesystem corruption.
2. Perform live-removal/full-media tests and representative sustained-load
   testing. Full-media and queue-overflow fault injection currently pass on host;
   real scheduler/board timing still needs measurement.
3. Confirm all required fields and sample rates with the team. Wire the actual
   producer to `flight_log_record` and select the custom board SPI/pins, RTOS
   configuration and readiness policy. Verify with the real sensor workload.
4. Review the published branch/PR and agree the remaining acceptance work before
   merging. The user has authorized publication, but not automatic merging.

The task may be handed over as an implementation/review milestone now with these
limits stated. It should not be marked fully accepted yet.
