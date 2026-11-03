/* Portable file policy only: serialization, session names, and durable boundaries.
 * The owner supplies storage callbacks; this core knows no SPI pins or RTOS API.
 */
#include "sd_logger.h"

#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char header[] =
    "log_format_version,packet_version,sequence_number,mission_time_ms,"
    "latitude_deg,longitude_deg,gps_altitude_m,barometric_altitude_m,pressure_pa,"
    "temperature_c,humidity_percent,vertical_speed_mps,battery_voltage_v,"
    "gps_fix_quality,gps_satellite_count,valid_fields,system_status\n";

static sd_log_result_t fault(sd_logger_t *logger, sd_log_result_t result)
{
    /* Latch the failure. Retrying is an application decision, never a hidden loop. */
    logger->status.state = SD_LOG_FAULT;
    logger->status.last_error = result;
    return result;
}

static sd_log_result_t write_all(sd_logger_t *logger, const char *data, size_t length)
{
    /* A successful API result with fewer bytes is still a failed record write. */
    size_t written = 0;
    sd_log_result_t result = logger->storage.write(logger->storage.context, data, length, &written);
    if (result != SD_LOG_OK) {
        return fault(logger, result);
    }
    if (written != length) {
        return fault(logger, SD_LOG_IO_ERROR);
    }
    return SD_LOG_OK;
}

sd_log_result_t sd_logger_init(sd_logger_t *logger, const sd_log_storage_t *storage)
{
    if (!logger || !storage || !storage->mount || !storage->create_new || !storage->write ||
        !storage->sync || !storage->close || !storage->now_ms) {
        return SD_LOG_INVALID;
    }
    sd_log_storage_t copy = *storage;
    memset(logger, 0, sizeof(*logger));
    logger->storage = copy;
    return SD_LOG_OK;
}

sd_log_result_t sd_logger_start(sd_logger_t *logger)
{
    if (!logger) {
        return SD_LOG_INVALID;
    }
    if (logger->opened || logger->status.state != SD_LOG_STOPPED) {
        return SD_LOG_NOT_READY;
    }
    memset(&logger->status, 0, sizeof(logger->status));
    sd_log_result_t result = logger->storage.mount(logger->storage.context);
    if (result != SD_LOG_OK) {
        return fault(logger, result);
    }
    /* Names fit FAT 8.3. Each callback must be bounded; worst-case scan latency
     * must be measured before flight use. No overwrite or persistent counter. */
    for (uint32_t index = 1; index <= 99999U; ++index) {
        (void)snprintf(logger->status.filename, sizeof(logger->status.filename),
                       "LOG%05" PRIu32 ".CSV", index);
        result = logger->storage.create_new(logger->storage.context, logger->status.filename);
        if (result == SD_LOG_EXISTS) {
            continue;
        }
        if (result != SD_LOG_OK) {
            return fault(logger, result);
        }
        logger->opened = true;
        result = write_all(logger, header, sizeof(header) - 1U);
        if (result != SD_LOG_OK) {
            return result;
        }
        /* Make the new file/header durable before accepting flight records. */
        result = logger->storage.sync(logger->storage.context);
        if (result != SD_LOG_OK) {
            return fault(logger, result);
        }
        logger->status.last_sync_ms = logger->storage.now_ms(logger->storage.context);
        logger->status.state = SD_LOG_READY;
        return SD_LOG_OK;
    }
    return fault(logger, SD_LOG_NAMES_EXHAUSTED);
}

/* Serialize a whole row before touching storage, so invalid input cannot write
 * half a row. GPS metadata uses bit 3; the nine floats use the other bits.
 * Do not dump struct memory: compiler padding is not an on-disk format.
 */
static sd_log_result_t serialize(sd_logger_t *logger, const hab_telemetry_record_t *record,
                                 size_t *length)
{
    if (!record || record->packet_version != HAB_TELEMETRY_PACKET_VERSION ||
        (record->valid_fields & ~UINT32_C(0x3ff)) != 0U) {
        return SD_LOG_INVALID;
    }
    /* Do not change process-global locale. Reject an incompatible environment. */
    if (strcmp(localeconv()->decimal_point, ".") != 0) {
        return SD_LOG_INVALID;
    }
    const float values[] = {
        record->latitude_deg,          record->longitude_deg,      record->gps_altitude_m,
        record->barometric_altitude_m, record->pressure_pa,        record->temperature_c,
        record->humidity_percent,      record->vertical_speed_mps, record->battery_voltage_v};
    const unsigned bits[] = {0, 1, 2, 4, 5, 6, 7, 8, 9};
    int count = snprintf(logger->row, sizeof(logger->row), "1,%u,%" PRIu32 ",%" PRIu32,
                         (unsigned)record->packet_version, record->sequence_number,
                         record->mission_time_ms);
    if (count < 0 || (size_t)count >= sizeof(logger->row)) {
        return SD_LOG_INVALID;
    }
    size_t used = (size_t)count;
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        if (record->valid_fields & (UINT32_C(1) << bits[i])) {
            if (!isfinite(values[i])) {
                return SD_LOG_INVALID;
            }
            count = snprintf(logger->row + used, sizeof(logger->row) - used, ",%.9g",
                             (double)values[i]);
        } else {
            count = snprintf(logger->row + used, sizeof(logger->row) - used, ",");
        }
        if (count < 0 || (size_t)count >= sizeof(logger->row) - used) {
            return SD_LOG_INVALID;
        }
        used += (size_t)count;
    }
    if (record->valid_fields & HAB_VALID_GPS_FIX_METADATA) {
        count = snprintf(logger->row + used, sizeof(logger->row) - used, ",%u,%u",
                         (unsigned)record->gps_fix_quality, (unsigned)record->gps_satellite_count);
    } else {
        count = snprintf(logger->row + used, sizeof(logger->row) - used, ",,");
    }
    if (count < 0 || (size_t)count >= sizeof(logger->row) - used) {
        return SD_LOG_INVALID;
    }
    used += (size_t)count;
    count = snprintf(logger->row + used, sizeof(logger->row) - used, ",%" PRIu32 ",%" PRIu32 "\n",
                     record->valid_fields, record->system_status);
    if (count < 0 || (size_t)count >= sizeof(logger->row) - used) {
        return SD_LOG_INVALID;
    }
    *length = used + (size_t)count;
    return SD_LOG_OK;
}

sd_log_result_t sd_logger_append(sd_logger_t *logger, const hab_telemetry_record_t *record)
{
    if (!logger) {
        return SD_LOG_INVALID;
    }
    if (logger->status.state != SD_LOG_READY) {
        return SD_LOG_NOT_READY;
    }
    size_t length;
    sd_log_result_t result = serialize(logger, record, &length);
    if (result != SD_LOG_OK) {
        logger->status.last_error = result;
        return result;
    }
    result = write_all(logger, logger->row, length);
    if (result != SD_LOG_OK) {
        return result;
    }
    /* Keep the oldest unsynced timestamp; later arrivals must not reset its age. */
    if (!logger->status.dirty) {
        logger->pending_oldest_ms = record->mission_time_ms;
    }
    logger->pending_sequence = record->sequence_number;
    logger->pending_mission_time_ms = record->mission_time_ms;
    ++logger->status.written_records;
    logger->status.dirty = true;
    return sd_logger_poll(logger);
}

sd_log_result_t sd_logger_sync(sd_logger_t *logger)
{
    if (!logger) {
        return SD_LOG_INVALID;
    }
    if (logger->status.state != SD_LOG_READY) {
        return SD_LOG_NOT_READY;
    }
    if (!logger->status.dirty) {
        return SD_LOG_OK;
    }
    sd_log_result_t result = logger->storage.sync(logger->storage.context);
    if (result != SD_LOG_OK) {
        return fault(logger, result);
    }
    /* Only successful sync advances the durable boundary. A failed callback
     * returns above without clearing dirty state or claiming records were saved. */
    logger->status.last_sync_ms = logger->storage.now_ms(logger->storage.context);
    uint32_t age = (uint32_t)(logger->status.last_sync_ms - logger->pending_oldest_ms);
    if (age > logger->status.max_age_at_sync_ms) {
        logger->status.max_age_at_sync_ms = age;
    }
    if (age > SD_LOGGER_LOSS_BUDGET_MS) {
        ++logger->status.missed_durability_deadlines;
    }
    logger->status.synced_records = logger->status.written_records;
    logger->status.last_synced_sequence = logger->pending_sequence;
    logger->status.last_synced_mission_time_ms = logger->pending_mission_time_ms;
    logger->status.has_synced_record = true;
    logger->status.dirty = false;
    logger->status.oldest_pending_age_ms = 0;
    return SD_LOG_OK;
}

sd_log_result_t sd_logger_poll(sd_logger_t *logger)
{
    if (!logger) {
        return SD_LOG_INVALID;
    }
    if (logger->status.state != SD_LOG_READY) {
        return SD_LOG_NOT_READY;
    }
    /* Service both timer age and queued-record age. Unsigned subtraction handles
     * clock wrap under the documented monotonic/half-range assumptions. */
    uint32_t now = logger->storage.now_ms(logger->storage.context);
    if (logger->status.dirty &&
        ((uint32_t)(now - logger->status.last_sync_ms) >= SD_LOGGER_SYNC_INTERVAL_MS ||
         (uint32_t)(now - logger->pending_oldest_ms) >= SD_LOGGER_SYNC_INTERVAL_MS)) {
        return sd_logger_sync(logger);
    }
    return SD_LOG_OK;
}

sd_log_result_t sd_logger_get_status(sd_logger_t *logger, sd_log_status_t *status)
{
    if (!logger || !status) {
        return SD_LOG_INVALID;
    }
    *status = logger->status;
    if (status->dirty) {
        status->oldest_pending_age_ms =
            (uint32_t)(logger->storage.now_ms(logger->storage.context) - logger->pending_oldest_ms);
    }
    return SD_LOG_OK;
}

sd_log_result_t sd_logger_stop(sd_logger_t *logger)
{
    if (!logger) {
        return SD_LOG_INVALID;
    }
    sd_log_result_t result =
        logger->status.state == SD_LOG_FAULT ? logger->status.last_error : SD_LOG_OK;
    if (logger->status.state == SD_LOG_READY) {
        result = sd_logger_sync(logger);
    }
    /* Release the handle even after failure, preserving the original error. */
    if (logger->opened) {
        sd_log_result_t close_result = logger->storage.close(logger->storage.context);
        logger->opened = false;
        if (result == SD_LOG_OK) {
            result = close_result;
        }
    }
    if (result != SD_LOG_OK) {
        return fault(logger, result);
    }
    logger->status.state = SD_LOG_STOPPED;
    return SD_LOG_OK;
}
