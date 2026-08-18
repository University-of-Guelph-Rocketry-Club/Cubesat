# HAB-001 software rehearsal

Before flight, rehearse this sequence with the intended mission configuration:

1. Power the STM32 and Pi Zero.
2. Verify STM32 self-test and preflight state.
3. Verify sensor and GPS validity reporting.
4. Verify primary telemetry independently of the Pi.
5. Verify STM32 records arrive at the Pi.
6. Verify camera capture and local storage.
7. Disconnect or stop the Pi and confirm the STM32 continues operating.
8. Restart the Pi and verify visible recovery behavior.
9. Run the complete payload for the planned mission duration plus margin.
