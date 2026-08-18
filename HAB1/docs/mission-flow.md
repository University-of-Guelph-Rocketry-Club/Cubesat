# Mission flow

The software-oriented mission sequence is:

```text
PRE-FLIGHT
    ↓
Power payload
    ↓
Verify sensors
    ↓
Verify logging
    ↓
Verify GPS
    ↓
Verify telemetry
    ↓
Verify Pi logger and camera
    ↓
LAUNCH
    ↓
Continuous STM32 sensing / state handling / telemetry
    ↓
Continuous Pi logging / camera capture
    ↓
BURST
    ↓
DESCENT
    ↓
LANDING
    ↓
Locate payload
    ↓
Recover SD/logged data
    ↓
Run post-flight analysis
```

The repository contains state sequencing but does not yet implement automatic launch, descent, or landing detectors. The STM32 must continue its critical responsibilities if the Pi logger fails. Tracking and recovery may use independent COTS systems and should remain possible if one telemetry path fails.
