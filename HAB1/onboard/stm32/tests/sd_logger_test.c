#include "../modules/sd_logger/sd_logger.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char data[8192];
    size_t used;
    unsigned files, creates, syncs, closes;
    uint32_t now;
    sd_log_result_t mount_error, create_error, write_error, sync_error, close_error;
    bool short_write, all_exist;
} fake_t;

static sd_log_result_t mount_card(void *p)
{
    return ((fake_t *)p)->mount_error;
}
static sd_log_result_t create_file(void *p, const char *name)
{
    fake_t *fixture = p;
    unsigned number = 0;
    assert(sscanf(name, "LOG%5u.CSV", &number) == 1);
    ++fixture->creates;
    if (fixture->create_error) {
        return fixture->create_error;
    }
    if (fixture->all_exist || number <= fixture->files) {
        return SD_LOG_EXISTS;
    }
    fixture->files = number;
    fixture->used = 0;
    fixture->data[0] = '\0';
    return SD_LOG_OK;
}
static sd_log_result_t write_file(void *p, const char *data, size_t length, size_t *written)
{
    fake_t *fixture = p;
    if (fixture->write_error) {
        return fixture->write_error;
    }
    *written = fixture->short_write ? length - 1U : length;
    assert(fixture->used + *written < sizeof(fixture->data));
    memcpy(fixture->data + fixture->used, data, *written);
    fixture->used += *written;
    fixture->data[fixture->used] = '\0';
    return SD_LOG_OK;
}
static sd_log_result_t sync_file(void *p)
{
    fake_t *fixture = p;
    ++fixture->syncs;
    return fixture->sync_error;
}
static sd_log_result_t close_file(void *p)
{
    fake_t *fixture = p;
    ++fixture->closes;
    return fixture->close_error;
}
static uint32_t now_ms(void *p)
{
    return ((fake_t *)p)->now;
}
static void init(sd_logger_t *logger, fake_t *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    sd_log_storage_t storage = {fixture,   mount_card, create_file, write_file,
                                sync_file, close_file, now_ms};
    assert(sd_logger_init(logger, &storage) == SD_LOG_OK);
}
static hab_telemetry_record_t make_record(void)
{
    hab_telemetry_record_t record = {0};
    record.packet_version = HAB_TELEMETRY_PACKET_VERSION;
    record.sequence_number = 7;
    record.temperature_c = 21.5f;
    record.valid_fields = HAB_VALID_TEMPERATURE_C;
    record.system_status = HAB_STATUS_BAROMETER_OK;
    return record;
}
static void session_and_csv(void)
{
    sd_logger_t logger;
    fake_t fixture;
    init(&logger, &fixture);
    fixture.files = 2;
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    assert(strcmp(logger.status.filename, "LOG00003.CSV") == 0);
    assert(fixture.syncs == 1 && !logger.status.has_synced_record);
    hab_telemetry_record_t record = make_record();
    size_t header_length = fixture.used;
    assert(sd_logger_append(&logger, &record) == SD_LOG_OK);
    assert(strcmp(fixture.data + header_length, "1,1,7,0,,,,,,21.5,,,,,,64,1\n") == 0);
    assert(logger.status.written_records == 1 && logger.status.synced_records == 0);
    fixture.now = 999;
    assert(sd_logger_poll(&logger) == SD_LOG_OK && fixture.syncs == 1);
    fixture.now = 1000;
    assert(sd_logger_poll(&logger) == SD_LOG_OK && fixture.syncs == 2);
    assert(logger.status.has_synced_record && logger.status.last_synced_sequence == 7);
    assert(sd_logger_stop(&logger) == SD_LOG_OK && fixture.closes == 1);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    assert(strcmp(logger.status.filename, "LOG00004.CSV") == 0);
    assert(logger.status.written_records == 0 && !logger.status.has_synced_record);
    assert(sd_logger_stop(&logger) == SD_LOG_OK);
    init(&logger, &fixture);
    fixture.now = 7000;
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    record.mission_time_ms = 1000;
    assert(sd_logger_append(&logger, &record) == SD_LOG_OK);
    assert(logger.status.max_age_at_sync_ms == 6000);
    assert(logger.status.missed_durability_deadlines == 1);
    assert(sd_logger_stop(&logger) == SD_LOG_OK);
}
static void validation(void)
{
    sd_logger_t logger;
    fake_t fixture;
    init(&logger, &fixture);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    size_t before = fixture.used;
    hab_telemetry_record_t record = make_record();
    record.packet_version = 99;
    assert(sd_logger_append(&logger, &record) == SD_LOG_INVALID && fixture.used == before);
    record = make_record();
    record.temperature_c = NAN;
    assert(sd_logger_append(&logger, &record) == SD_LOG_INVALID && fixture.used == before);
    record.valid_fields = 0; /* Invalid-marked NaNs are omitted, not invented as zero. */
    assert(sd_logger_append(&logger, &record) == SD_LOG_OK);
    record.valid_fields = UINT32_MAX;
    assert(sd_logger_append(&logger, &record) == SD_LOG_INVALID);
    assert(sd_logger_append(&logger, NULL) == SD_LOG_INVALID);
    record = make_record();
    record.valid_fields = 0x3ff;
    record.latitude_deg = FLT_MAX;
    record.longitude_deg = -FLT_MAX;
    record.gps_fix_quality = 2;
    record.gps_satellite_count = 12;
    record.sequence_number = UINT32_MAX;
    assert(sd_logger_append(&logger, &record) == SD_LOG_OK);
    assert(strstr(fixture.data, ",2,12,1023,1\n"));
    assert(sd_logger_stop(&logger) == SD_LOG_OK);
}
static void failures(void)
{
    sd_logger_t logger;
    fake_t fixture;
    hab_telemetry_record_t record = make_record();
    init(&logger, &fixture);
    fixture.mount_error = SD_LOG_IO_ERROR;
    assert(sd_logger_start(&logger) == SD_LOG_IO_ERROR && !logger.opened);
    init(&logger, &fixture);
    fixture.create_error = SD_LOG_FULL;
    assert(sd_logger_start(&logger) == SD_LOG_FULL && !logger.opened);
    init(&logger, &fixture);
    fixture.all_exist = true;
    assert(sd_logger_start(&logger) == SD_LOG_NAMES_EXHAUSTED && fixture.creates == 99999);
    init(&logger, &fixture);
    fixture.short_write = true;
    assert(sd_logger_start(&logger) == SD_LOG_IO_ERROR);
    assert(sd_logger_stop(&logger) == SD_LOG_IO_ERROR && fixture.closes == 1);
    init(&logger, &fixture);
    fixture.sync_error = SD_LOG_IO_ERROR;
    assert(sd_logger_start(&logger) == SD_LOG_IO_ERROR && logger.opened);
    assert(sd_logger_stop(&logger) == SD_LOG_IO_ERROR && fixture.closes == 1);
    init(&logger, &fixture);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    fixture.short_write = true;
    assert(sd_logger_append(&logger, &record) == SD_LOG_IO_ERROR);
    assert(logger.status.written_records == 0 && !logger.status.has_synced_record);
    assert(sd_logger_stop(&logger) == SD_LOG_IO_ERROR);
    init(&logger, &fixture);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    fixture.write_error = SD_LOG_FULL;
    assert(sd_logger_append(&logger, &record) == SD_LOG_FULL);
    assert(logger.status.written_records == 0);
    assert(sd_logger_stop(&logger) == SD_LOG_FULL && fixture.closes == 1);
    init(&logger, &fixture);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    assert(sd_logger_append(&logger, &record) == SD_LOG_OK);
    fixture.sync_error = SD_LOG_IO_ERROR;
    fixture.now = 1000;
    assert(sd_logger_poll(&logger) == SD_LOG_IO_ERROR);
    assert(logger.status.dirty && !logger.status.has_synced_record &&
           logger.status.synced_records == 0);
    sd_log_status_t status;
    assert(sd_logger_get_status(&logger, &status) == SD_LOG_OK);
    assert(status.oldest_pending_age_ms == 1000);
    assert(sd_logger_append(&logger, &record) == SD_LOG_NOT_READY);
    assert(sd_logger_stop(&logger) == SD_LOG_IO_ERROR && fixture.closes == 1);
    init(&logger, &fixture);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    fixture.close_error = SD_LOG_IO_ERROR;
    assert(sd_logger_stop(&logger) == SD_LOG_IO_ERROR && !logger.opened);
}
static void float_round_trip_and_shutdown(void)
{
    sd_logger_t logger;
    fake_t fixture;
    init(&logger, &fixture);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    size_t offset = fixture.used;
    hab_telemetry_record_t record = make_record();
    record.latitude_deg = 43.5448f;
    record.longitude_deg = -80.2482f;
    record.gps_altitude_m = 12345.678f;
    record.barometric_altitude_m = -12.345f;
    record.pressure_pa = 101325.0f;
    record.temperature_c = -0.000123456789f;
    record.humidity_percent = 52.125f;
    record.vertical_speed_mps = -1.2345678f;
    record.battery_voltage_v = 4.1234567f;
    record.valid_fields = 0x3ff;
    const float expected[] = {
        record.latitude_deg,          record.longitude_deg,      record.gps_altitude_m,
        record.barometric_altitude_m, record.pressure_pa,        record.temperature_c,
        record.humidity_percent,      record.vertical_speed_mps, record.battery_voltage_v};
    assert(sd_logger_append(&logger, &record) == SD_LOG_OK);
    char *field = fixture.data + offset;
    for (unsigned i = 0; i < 4; ++i) {
        field = strchr(field, ',');
        assert(field);
        ++field;
    }
    for (unsigned i = 0; i < 9; ++i) {
        char *end;
        float value = strtof(field, &end);
        assert(value == expected[i] && *end == ',');
        field = end + 1;
    }
    assert(!logger.status.has_synced_record);
    assert(sd_logger_stop(&logger) == SD_LOG_OK);
    assert(fixture.syncs == 2 && fixture.closes == 1 && logger.status.synced_records == 1);
}
static void wrap_and_continuous_load(void)
{
    sd_logger_t logger;
    fake_t fixture;
    init(&logger, &fixture);
    fixture.now = UINT32_MAX - 499U;
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    hab_telemetry_record_t record = make_record();
    record.mission_time_ms = fixture.now;
    assert(sd_logger_append(&logger, &record) == SD_LOG_OK);
    fixture.now = 500;
    assert(sd_logger_poll(&logger) == SD_LOG_OK && fixture.syncs == 2);
    /* Continuous appends also service sync; polling is required while idle. */
    for (unsigned i = 0; i < 20; ++i) {
        fixture.now += 100;
        record.mission_time_ms = fixture.now;
        ++record.sequence_number;
        assert(sd_logger_append(&logger, &record) == SD_LOG_OK);
    }
    assert(fixture.syncs == 4);
    assert(sd_logger_stop(&logger) == SD_LOG_OK);
    /* A record delayed in a queue must trigger sync immediately. */
    init(&logger, &fixture);
    fixture.now = 4000;
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    record.mission_time_ms = 2000;
    assert(sd_logger_append(&logger, &record) == SD_LOG_OK && fixture.syncs == 2);
    assert(sd_logger_stop(&logger) == SD_LOG_OK);
}
int main(void)
{
    session_and_csv();
    validation();
    failures();
    float_round_trip_and_shutdown();
    wrap_and_continuous_load();
    puts("PASS: sd_logger CSV, sessions, deadlines, wrap and storage failures");
    return 0;
}
