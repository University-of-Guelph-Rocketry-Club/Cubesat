# Drivers

Create one device directory per selected subsystem. A typical driver API should provide explicit initialization, non-blocking polling, and retrieval of the latest validated result.

Suggested locations:

```text
barometer/
gps/
temperature/
power/
telemetry_radio/
pi_link/
```

Do not put mission-state decisions, hard-coded board pins, or a private application loop inside a driver.
