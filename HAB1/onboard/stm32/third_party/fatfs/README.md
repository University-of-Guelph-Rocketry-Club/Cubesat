# FatFs R0.16

Upstream: https://elm-chan.org/fsw/ff/arc/ff16.zip
Files: `ff.c`, `ff.h`, `diskio.h`, `ffconf.h`, `00readme.txt`.
Upstream copyright and redistribution terms are preserved in the source headers.

Only `ffconf.h` is customized: OEM code page 437 and no RTC. Other defaults
retain FAT12/16/32, one drive, 512-byte sectors, 8.3 names, no dynamic LFN buffers,
no exFAT, no mkfs, no TRIM, and no filesystem reentrancy.

All filesystem access belongs to one logging task; host tests provide isolated
RAM media. No formatting API is compiled into the target. Filesystem timestamps
use the configured fixed date and do not represent the actual flight time;
record mission timestamps are authoritative.
