# Integrated STM32 flight application

This is the final STM32 FreeRTOS application for HAB-001. It owns initialization, creation of statically allocated tasks, and scheduler startup while reusable behavior remains in `modules/`.

`main.c` starts FreeRTOS and delegates task construction to `flight_tasks.c`. The task scaffold does not access unselected hardware or invent state-detection thresholds. A target build requires an approved STM32 port and FreeRTOS headers.
