# Testing progression

Test the system in increasing order of realism and risk:

```text
unit test
↓
sensor bench test
↓
integrated payload test
↓
long-duration test
↓
cold/environment test
↓
radio range test
↓
full mission rehearsal
↓
flight
```

## What each stage checks

1. **Unit test:** calculations, parsing, telemetry validity, state transitions, and error handling without hardware.
2. **Sensor bench test:** one selected sensor at a time, including expected ranges and disconnected/failure behavior.
3. **Integrated payload test:** STM32 sensing/state handling, record construction, Pi logging/camera operation, telemetry, and health status together.
4. **Long-duration test:** storage growth, power behavior, timing, watchdog behavior, and recovery after transient errors.
5. **Cold/environment test:** startup, sensor readings, storage, and power behavior under expected environmental conditions.
6. **Radio range test:** packet reception, loss behavior, decoding, and ground logging at representative distances while Pi logging remains independent.
7. **Full mission rehearsal:** pre-flight checklist through recovery and post-flight analysis using the intended configuration.
8. **Flight:** only after results, configuration, wiring revision, and known limitations are recorded.

Hardware procedures live in [`tests/hardware/`](../tests/hardware/). Small parser and telemetry fixtures live in [`tests/data/`](../tests/data/). The complete STM32/Pi rehearsal lives in [`onboard/integration/HAB-001/`](../onboard/integration/HAB-001/).
