# Protocols

The logical telemetry record is defined in [`shared/telemetry/telemetry_record.h`](../telemetry/telemetry_record.h). A binary packet layout, checksum/CRC, radio framing, and command uplink are intentionally not selected yet.

- `stm32-pi/` owns framing shared by the flight controller and onboard logger.
- `radio/` owns framing shared by the flight controller and ground receiver.

When a wire protocol is added:

1. include the packet version in every packet;
2. document field order, byte order, sizes, units, and checksum behavior here;
3. reject unsupported versions visibly; and
4. keep recorded packets and decoders version-aware so old mission data remains understandable.
