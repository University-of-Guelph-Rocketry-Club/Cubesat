# NUCLEO-F411RE SD write/read bench test

The completed C/HAL/FatFs logger bench lives under `c_logger/`; see
[integration](../../../../../docs/sd-logging-integration.md) and
[results](../../../../../docs/sd-logging-results.md). The original Arduino test
below remains a comparison tool. `sd_recovery/` is an Arduino read-only file
inspection aid and never formats, repairs, deletes, or creates files.

This standalone Arduino sketch verifies the adapter, wiring, and filesystem.
It does not implement the proposed C/FreeRTOS flight logger.

## Wiring and card

| Module | Nucleo |
| --- | --- |
| VCC | 5V, for the confirmed 5V-input regulated module |
| GND | GND |
| SCK | D13 / PA5 |
| MISO | D12 / PA6 |
| MOSI | D11 / PA7 |
| CS | D10 / PB6 |

Use an existing FAT16/FAT32 filesystem. The test does not format the card or
delete files. A nominal 1 GB card is suitable. The ST-Link USB drive is a
programming interface, not a reader for the external SD card.

## Build and flash

Install Arduino CLI, the STMicroelectronics STM32 core, and Arduino SD library.
The STM32 package index is
`https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json`.
From this directory:

```powershell
arduino-cli compile --fqbn STMicroelectronics:stm32:Nucleo_64:pnum=NUCLEO_F411RE --output-dir build sd_card_test
```

Connect the Nucleo's ST-Link USB connector. Copy the generated `.bin` file to
the verified `NOD_F411RE` drive to program the board. This replaces the firmware
currently on the Nucleo. Do not copy it to the external SD card.

Open the ST-Link virtual serial port at 115200 baud, then press RESET.
Each boot chooses an unused `TEST0000.TXT` through `TEST9999.TXT`, writes a
known message, flushes/closes, reopens, and checks exact contents and length.
No existing test file is overwritten. Filename probing is for this single-owner
bench test, not the final exclusive-create flight implementation.

Expected serial result: `PASS: wrote, closed, reopened and verified TESTxxxx.TXT`.
Slow LED blinking means pass; fast blinking means failure. The LED shares SCK,
so blinking starts only after all card operations finish. Serial gives the
failure stage. Reopening verifies the software path, not power-cut durability;
check the physical card with a separate reader after disconnecting board power.

Record actual board/card, serial output, and file contents before marking this
test passed. Compilation alone is not a hardware pass.

## First bench result

Compiled with STMicroelectronics STM32 core 3.0.0 and Arduino SD 1.3.0,
then flashed through the ST-Link programming drive. Serial output on COM7:

```text
URC-43 NUCLEO-F411RE SD bench test
Card error code=0x1 response=0xFF
FAIL: SPI card initialization; check pin positions, power and seating
```

Error 0x1 is the SD library's CMD0 initialization failure. No recognizable card
response was received at that stage; this does not identify a single hardware
cause. No test file was created. Inspect wiring, supply, and card seating before
changing the filesystem. The diagnostic retry distinguishes card initialization
from a later FAT volume mount failure.

## Successful retest after wiring correction

The user identified signals on the wrong CN10 column (NC/PA12/PA11/PB12) and
moved them to PA5/PA6/PA7/PB6, CN10 pins 11/13/15/17 respectively. With the
same test binary, serial output confirmed:

```text
URC-43 NUCLEO-F411RE SD bench test
PASS: wrote, closed, reopened and verified TEST0001.TXT
URC-43: SD write/read test passed.
Bench check only; power-cut durability is not tested.
```

This verifies initialization, file creation, write, close, reopen, and exact
read-back on the bench setup. It does not establish power-cut durability.
