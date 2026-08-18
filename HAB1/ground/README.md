# Ground software

Ground software runs on a laptop or other computer after launch. It may use richer libraries than flight firmware, but the initial repository keeps the tools command-line first.

- `telemetry/` will receive, validate, decode, and record live packets.
- `visualization/` will provide optional plots and views.
- `recovery/` will support location extraction, log conversion, and post-flight analysis.
