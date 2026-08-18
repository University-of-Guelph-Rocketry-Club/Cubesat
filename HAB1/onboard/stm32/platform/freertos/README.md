# FreeRTOS platform configuration

FreeRTOS is the selected STM32 execution environment. This directory will own or reference the approved kernel version, STM32 port, `FreeRTOSConfig.h`, allocation policy, tick source, interrupt-priority rules, and runtime statistics configuration.

The current task scaffolds require `configSUPPORT_STATIC_ALLOCATION=1`. Task priorities, stack depths, queue lengths, tick frequency, scheduling/preemption settings, and watchdog deadlines must be measured and reviewed after the STM32 target and workloads are selected.

Do not commit an arbitrary generated FreeRTOS configuration before the exact STM32 family and integration route are approved.
