# Tools

Tools support development and mission analysis without becoming flight firmware.

- `log_parser/` will convert onboard logs into useful formats such as CSV.
- `telemetry_decoder/` will decode recorded raw packets and report malformed or unsupported packets.
- `simulation/` will generate simple ground/ascent/burst/descent/landing sequences for ground-tool testing.

Python is an appropriate future implementation language for these utilities.
