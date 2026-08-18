# Radio telemetry protocol

This directory will define versioned framing shared by the STM32 telemetry application and ground receiver once the radio is selected.

Keep radio framing separate from `hab_telemetry_record_t`; do not transmit the compiler's in-memory structure directly.
