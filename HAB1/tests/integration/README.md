# Integration tests

Planned software and payload integration flows:

```text
sensor sample
→ telemetry record
→ encode
→ decode
→ compare original values and validity flags

STM32 record
→ versioned STM32-to-Pi frame
→ Pi raw-frame storage
→ decoded index plus camera timestamp
→ post-flight correlation
```

Add protocol tests when a wire format and decoder exist. Use `onboard/integration/HAB-001/rehearsal.md` for the full two-computer payload sequence.
