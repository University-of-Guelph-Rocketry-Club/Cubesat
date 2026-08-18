#ifndef HAB_TELEMETRY_RECORD_H
#define HAB_TELEMETRY_RECORD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAB_TELEMETRY_PACKET_VERSION UINT8_C(1)

/* Bits set in valid_fields identify validated measurements. */
#define HAB_VALID_LATITUDE_DEG          (UINT32_C(1) << 0)
#define HAB_VALID_LONGITUDE_DEG         (UINT32_C(1) << 1)
#define HAB_VALID_GPS_ALTITUDE_M        (UINT32_C(1) << 2)
#define HAB_VALID_GPS_FIX_METADATA      (UINT32_C(1) << 3)
#define HAB_VALID_BAROMETRIC_ALTITUDE_M (UINT32_C(1) << 4)
#define HAB_VALID_PRESSURE_PA           (UINT32_C(1) << 5)
#define HAB_VALID_TEMPERATURE_C         (UINT32_C(1) << 6)
#define HAB_VALID_HUMIDITY_PERCENT      (UINT32_C(1) << 7)
#define HAB_VALID_VERTICAL_SPEED_MPS    (UINT32_C(1) << 8)
#define HAB_VALID_BATTERY_VOLTAGE_V     (UINT32_C(1) << 9)

/* Bits set in system_status identify healthy or ready subsystems. */
#define HAB_STATUS_BAROMETER_OK          (UINT32_C(1) << 0)
#define HAB_STATUS_TEMPERATURE_SENSOR_OK (UINT32_C(1) << 1)
#define HAB_STATUS_GPS_OK                (UINT32_C(1) << 2)
#define HAB_STATUS_PI_LINK_OK            (UINT32_C(1) << 3)
#define HAB_STATUS_TELEMETRY_OK          (UINT32_C(1) << 4)
#define HAB_STATUS_BATTERY_MONITOR_OK    (UINT32_C(1) << 5)
#define HAB_STATUS_WATCHDOG_READY        (UINT32_C(1) << 6)

typedef struct {
    uint8_t packet_version;
    uint32_t sequence_number;
    uint32_t mission_time_ms;

    float latitude_deg;
    float longitude_deg;
    float gps_altitude_m;
    float barometric_altitude_m;
    float pressure_pa;
    float temperature_c;
    float humidity_percent;
    float vertical_speed_mps;
    float battery_voltage_v;

    uint8_t gps_fix_quality;
    uint8_t gps_satellite_count;

    uint32_t valid_fields;
    uint32_t system_status;
} hab_telemetry_record_t;

#ifdef __cplusplus
}
#endif

#endif
