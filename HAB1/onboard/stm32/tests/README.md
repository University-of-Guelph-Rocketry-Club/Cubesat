# STM32 tests

Host tests should cover state transitions, conversions, framing, parsing, and validity behavior. Target tests should cover board startup, buses, watchdog, timing, and each selected hardware module.

The initial state-machine test can be built and run on a development computer:

```bash
cc -std=c11 -Wall -Wextra -Werror \
  modules/state_machine/state_machine.c \
  tests/state_machine_test.c \
  -o /tmp/hab_state_machine_test
/tmp/hab_state_machine_test
```
