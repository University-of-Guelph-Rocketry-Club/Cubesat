# Mission state machine

The state machine sequences explicit mission states. It consumes tested detector and health inputs; it does not contain sensor drivers or invent thresholds.

Initial states are `BOOT`, `SELF_TEST`, `PREFLIGHT`, `ASCENT`, `DESCENT`, `LANDED`, and `FAULT`.
