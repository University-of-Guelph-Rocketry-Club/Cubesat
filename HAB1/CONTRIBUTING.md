# Contributing

This is a student project. Prefer clear, small changes that another student can understand and test.

## Branches

Use descriptive branches such as:

```text
feature/barometer-driver
feature/gps-parser
feature/telemetry-decoder
fix/sd-logging
docs/gps-testing
```

## Pull requests

Keep a pull request focused on one change when practical. Include:

- What changed?
- Why was it needed?
- How was it tested?
- Does it affect flight hardware?

If hardware is involved, include the board or device model, wiring changes, and the bench-test result. Do not present unverified wiring or sensor values as flight-ready.

## Coding principles

- Prefer readable code over clever code.
- Put units in names such as `pressure_pa` and `mission_time_ms`.
- Keep STM32 flight code deterministic C using the approved FreeRTOS task architecture.
- Prefer static task, queue, and event-group allocation for flight builds.
- Keep interrupt handlers short; use task notifications or queues to hand work to tasks.
- Give the mission state machine one owning task instead of mutating it from multiple tasks.
- Put reusable subsystem code in `onboard/*/modules/`; keep bring-up `main` files small.
- Do not copy a subsystem implementation into the final integration application.
- Preserve raw measurements alongside derived values where practical.
- Expose sensor failures instead of silently producing plausible values.
- Keep COTS avionics independent unless an intentional data interface is documented.
