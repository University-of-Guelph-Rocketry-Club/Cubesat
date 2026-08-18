# Mission state machine

The state machine sequences explicit mission states. It consumes tested detector and health inputs; it does not contain sensor drivers or invent thresholds.

Exactly one FreeRTOS state-management task owns and steps the state-machine object. Other tasks provide immutable snapshots or events through queues, notifications, or event groups.

Initial states are `BOOT`, `SELF_TEST`, `PREFLIGHT`, `ASCENT`, `DESCENT`, `LANDED`, and `FAULT`.
