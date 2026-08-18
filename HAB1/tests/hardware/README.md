# Hardware tests

Hardware test procedures may be checklists rather than automated tests. Initial procedures should cover:

- barometer sanity;
- GPS lock and parsed position;
- Pi camera capture and storage;
- STM32-to-Pi communication and Pi-disconnect behavior;
- optional STM32 SD-card file creation and append, if fitted;
- battery-voltage measurement;
- cold/environment exposure;
- radio range; and
- long-duration logging.

Record board, device, wiring revision, date, result, and observed failures for every bench test.
