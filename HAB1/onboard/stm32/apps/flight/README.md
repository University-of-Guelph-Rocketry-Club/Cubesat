# Integrated STM32 flight application

This is the final STM32 application for HAB-001. It owns initialization order and the cooperative superloop while reusable behavior remains in `modules/`.

The current `main.c` is a compilable C skeleton. It does not access unselected hardware or invent state-detection thresholds.
