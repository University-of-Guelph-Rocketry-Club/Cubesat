# STM32-to-Pi protocol

This directory is the canonical home for the versioned link between the STM32 flight controller and Pi Zero logger.

The first approved frame should define:

- protocol version and message type;
- sequence number and STM32 mission time;
- payload length and byte order;
- bounded maximum frame size;
- checksum or CRC behavior;
- timeout and resynchronization behavior; and
- what the STM32 does when the Pi is absent or slow.

The physical transport is still `TBD`. The STM32 must never block its flight loop indefinitely while waiting for the Pi.
