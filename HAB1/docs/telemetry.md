# Telemetry format

The canonical C-compatible logical record is [`shared/telemetry/telemetry_record.h`](../shared/telemetry/telemetry_record.h). It is shared by the STM32, Pi Zero logger, and ground software. The current logical packet version is `1`.

Every record includes `packet_version`, `sequence_number`, `mission_time_ms`, `valid_fields`, and `system_status`. A field is meaningful only when its validity bit is set. Unknown packet versions must be rejected visibly; decoders must not silently reinterpret fields.

## Fields

| Field | Type | Units | Description | Validity/range |
| --- | --- | --- | --- | --- |
| `packet_version` | `uint8_t` | version | Logical telemetry schema version | Must be supported by the reader; current value `1` |
| `sequence_number` | `uint32_t` | count | Monotonically increasing record number | Wrap behavior to be documented when needed |
| `mission_time_ms` | `uint32_t` | ms | Time since mission software start | Monotonic while the counter is valid |
| `latitude_deg` | `float` | degrees | GPS latitude | `[-90, 90]` when `HAB_VALID_LATITUDE_DEG` is set |
| `longitude_deg` | `float` | degrees | GPS longitude | `[-180, 180]` when `HAB_VALID_LONGITUDE_DEG` is set |
| `gps_altitude_m` | `float` | m | GPS-reported altitude | Receiver-specific validity and range |
| `barometric_altitude_m` | `float` | m | Altitude derived from pressure | Valid only with a valid pressure/calibration basis |
| `pressure_pa` | `float` | Pa | Raw barometric pressure | Positive, sensor-range checked |
| `temperature_c` | `float` | °C | Raw temperature measurement | Sensor-range checked |
| `humidity_percent` | `float` | % | Optional humidity measurement | `[0, 100]` when hardware exists and is valid |
| `vertical_speed_mps` | `float` | m/s | Derived vertical speed | Requires documented derivation and valid source data |
| `battery_voltage_v` | `float` | V | Battery or power-rail voltage | Divider/monitor range checked |
| `gps_fix_quality` | `uint8_t` | code | GPS receiver fix quality | Receiver-specific codes |
| `gps_satellite_count` | `uint8_t` | count | Satellites used/reported by GPS | Valid only with GPS metadata |
| `valid_fields` | `uint32_t` | bitmask | Which measurement fields are valid | See `HAB_VALID_*` constants in the shared header |
| `system_status` | `uint32_t` | bitmask | Health of sensors, Pi link, telemetry, battery monitor, and watchdog | See `HAB_STATUS_*` constants in the shared header |

Optional fields may remain invalid when the corresponding hardware is not installed. The system must not populate them with fake values merely to make a packet look complete.

## Wire protocol status

The repository currently defines the logical record only. Do not transmit the compiler's structure memory directly. Binary layout, byte order, framing, CRC, radio settings, and command uplink remain protocol decisions documented under [`shared/protocols/`](../shared/protocols/).
