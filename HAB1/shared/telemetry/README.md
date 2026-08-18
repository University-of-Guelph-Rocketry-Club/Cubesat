# Telemetry record

[`telemetry_record.h`](telemetry_record.h) is the canonical C-compatible logical record for HAB software. It uses explicit units and fixed-width integer types so the STM32, Pi Zero, and ground decoders can agree on field meaning before a binary wire format is selected.

Fields that do not have installed hardware must remain invalid according to `valid_fields`; their numeric storage is not meaningful. Raw pressure and temperature are retained alongside derived altitude and vertical speed where available.

The current packet version is `1`. Readers must check the version and fail visibly for an unsupported version rather than silently guessing field meanings.

The C structure is a logical in-memory record, not a wire packet. Do not transmit `sizeof(hab_telemetry_record_t)` bytes directly because compiler padding, byte order, and floating-point representation are not yet part of an approved protocol.
