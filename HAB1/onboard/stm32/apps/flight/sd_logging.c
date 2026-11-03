/* Flight-side ownership boundary: producers copy records into a static queue;
 * the logger task alone calls the storage core. No card I/O in critical sections.
 */
#include "sd_logging.h"
#include "queue.h"
#include "task.h"
#include <string.h>

/* Storage and control blocks live for the full mission; no heap-backed queue. */
static StaticQueue_t queue_control;
static uint8_t queue_data[HAB_SD_QUEUE_LENGTH * sizeof(hab_telemetry_record_t)];
static QueueHandle_t queue;
static sd_logger_t logger;
static hab_sd_status_t shared;
static bool configured;

static void publish_status(void)
{
    sd_log_status_t snapshot;
    (void)sd_logger_get_status(&logger, &snapshot);
    /* Obtain the owner-only core snapshot before briefly locking the shared copy. */
    taskENTER_CRITICAL();
    shared.storage = snapshot;
    shared.ready = snapshot.state == SD_LOG_READY;
    taskEXIT_CRITICAL();
}
BaseType_t hab_sd_configure(const sd_log_storage_t *storage)
{
    if (configured || sd_logger_init(&logger, storage) != SD_LOG_OK) {
        return pdFAIL;
    }
    memset(&shared, 0, sizeof(shared));
    queue = xQueueCreateStatic(HAB_SD_QUEUE_LENGTH, sizeof(hab_telemetry_record_t), queue_data,
                               &queue_control);
    configured = queue != NULL;
    return configured ? pdPASS : pdFAIL;
}
sd_log_result_t hab_sd_start(void)
{
    if (!configured) {
        return SD_LOG_NOT_READY;
    }
    sd_log_result_t result = sd_logger_start(&logger);
    if (result != SD_LOG_OK) {
        (void)sd_logger_stop(&logger);
    }
    publish_status();
    return result;
}
sd_log_result_t hab_sd_submit(const hab_telemetry_record_t *record)
{
    if (!configured) {
        return SD_LOG_NOT_READY;
    }
    if (!record || record->packet_version != HAB_TELEMETRY_PACKET_VERSION ||
        (record->valid_fields & ~UINT32_C(0x3ff))) {
        taskENTER_CRITICAL();
        ++shared.invalid;
        taskEXIT_CRITICAL();
        return SD_LOG_INVALID;
    }
    /* Serialize readiness check, enqueue and counters so status stays consistent.
     * A zero wait guarantees the producer cannot block on a slow card. */
    sd_log_result_t result;
    taskENTER_CRITICAL();
    if (!shared.ready) {
        ++shared.dropped;
        result = SD_LOG_NOT_READY;
    } else if (xQueueSend(queue, record, 0) != pdPASS) {
        ++shared.dropped;
        result = SD_LOG_FULL;
    } else {
        ++shared.accepted;
        UBaseType_t used = uxQueueMessagesWaiting(queue);
        if (used > shared.queue_high_water) {
            shared.queue_high_water = used;
        }
        result = SD_LOG_OK;
    }
    taskEXIT_CRITICAL();
    return result;
}
sd_log_result_t hab_sd_service(void)
{
    if (!configured) {
        return SD_LOG_NOT_READY;
    }
    if (logger.status.state != SD_LOG_READY) {
        publish_status();
        return SD_LOG_NOT_READY;
    }
    /* One record per service call bounds queue work and prevents poll starvation.
     * The flight task currently calls this once per nominal 10ms cycle. */
    hab_telemetry_record_t record;
    sd_log_result_t result = SD_LOG_OK;
    if (xQueueReceive(queue, &record, 0) == pdPASS) {
        result = sd_logger_append(&logger, &record);
        if (result == SD_LOG_INVALID) {
            taskENTER_CRITICAL();
            ++shared.invalid;
            taskEXIT_CRITICAL();
        }
    }
    if (logger.status.state == SD_LOG_READY) {
        sd_log_result_t poll_result = sd_logger_poll(&logger);
        if (poll_result != SD_LOG_OK) {
            result = poll_result;
        }
    }
    if (logger.status.state == SD_LOG_FAULT) {
        /* Stop accepting data before potentially slow cleanup. */
        publish_status();
        (void)sd_logger_stop(&logger);
    }
    publish_status();
    return result;
}
void hab_sd_status(hab_sd_status_t *status)
{
    if (!status) {
        return;
    }
    taskENTER_CRITICAL();
    *status = shared;
    taskEXIT_CRITICAL();
}
