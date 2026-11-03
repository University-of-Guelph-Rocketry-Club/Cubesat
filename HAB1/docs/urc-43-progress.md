# URC-43 assignment checklist

- [x] Define proposed shared-record interface and document open team decisions.
- [x] SD bench test: compile, flash, write/read verification on Nucleo.
- [x] Implement portable C logging core with storage callbacks and host tests.
- [x] Connect a C filesystem/SPI adapter to the logging core; normal bench run passed.
- [x] Create unique session file per boot and implement periodic sync; reset tests passed.
- [x] Implement optional queue/task and status integration; host tests and ARM compile passed.
- [x] Validate normal resets, physical missing-card startup/reinsertion, and three recorded power-cut trials.
- [ ] Validate physical live removal/full media and the final agreed flight workload.
- [x] Document implementation, integration steps, and measured software/bench results.
- [x] Comment/format authored code and add the complete `WHAT_WE_ADDED.md` inventory.
- [x] Recover and validate all interrupted sessions; two additional cuts recovered automatically.
- [ ] Characterize earlier intermittent startup behavior on the final hardware if it recurs.
- [x] User authorized publishing the feature branch and opening a review PR; delivery tracked in the chat.

Current: C/FatFs/SPI implementation and optional flight task/queue are implemented.
Latest hardware update: all 14 CSV files recovered and independently verified.
Two additional cuts saved all 40 and 32 captured records, recovered automatically,
and preserved earlier checksums. Missing-card startup and reinsertion also passed.
The board is currently running read-only recovery inspection firmware.
All four host suites passed; enabled integration compiles with FreeRTOS V11.2.0
Cortex-M4F headers. Two captured C-only bench sessions each verified 12 exact
synthetic rows; maximum observed sample-to-sync age was 1011 ms. Earlier session
checksums survived resets. Real sensor record assembly and custom-board settings
remain team dependencies.

Physical power-cut trial is unresolved: continuous LOG00004.CSV emitted through
sequence 9, then reported a sync fault around the user-confirmed USB disconnect.
After reconnect, C initialization failed (CMD0 no response). Repeating a full
power-off/reseat did not restore the C path. The original Arduino test initialized
the card but failed file creation. A subsequent read-only Arduino inspection
also failed initialization. The board is left running that read-only firmware.
Do not claim the five-second power-cut requirement passed.
Later operator restart restored read-only access: all four logs were recovered.
Earlier-session checksums match; the interrupted file contains exact records
0-8, missing only captured sample 9 (1005 ms sample-timestamp gap). The electrical
cutoff time was not measured, and intermittent startup remains unresolved.
See [measured results and remaining checks](sd-logging-results.md).
After the later computer restart, the current C firmware passed a new 12-record
write/sync/read test to LOG00005.CSV, preserving all earlier session checksums.
All four host suites and ARM checks passed again. The board now runs the C bench
firmware. See the [review handoff](urc-43-handoff.md) before task submission.
Two further reset/write/read runs passed to LOG00006.CSV and LOG00007.CSV,
with all earlier checksums preserved and maximum observed sync age 1011 ms.
## Earlier Arduino-only milestone

The standalone Arduino bench test passed after correcting the CN10 header row.
Earlier connections used NC/PA12/PA11/PB12 instead of the SPI1 pins.
The user moved signals to PA5/PA6/PA7/PB6 (CN10 11/13/15/17).
Captured serial result: `PASS: wrote, closed, reopened and verified TEST0001.TXT`.
The known message matched exactly. That earlier test did not exercise the C
logger's periodic sync or power-cut behavior.
Build: STM32 Arduino core 3.0.0, Arduino SD 1.3.0; 24,884 bytes flash,
2,048 bytes static RAM. Test source and setup are in
`onboard/stm32/apps/bringup/sd_card/`.
Bench hardware: NUCLEO-F411RE, nominal 1 GB card, regulated 5V-input SPI adapter;
user confirmed 5V supply and card inserted. SPI wiring is D13/D12/D11 with CS D10.
Windows identifies ST-Link serial as COM7 and programming drive as NOD_F411RE.
Working branch: `codex/urc-43-sd-logging`. Final flight-board validation and
agreement on full data coverage remain required.
