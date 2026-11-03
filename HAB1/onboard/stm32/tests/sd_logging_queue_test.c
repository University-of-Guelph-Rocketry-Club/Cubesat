#include "../apps/flight/sd_logging.h"
#include "queue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned critical;
void test_enter_critical(void)
{
    ++critical;
}
void test_exit_critical(void)
{
    assert(critical);
    --critical;
}
QueueHandle_t xQueueCreateStatic(UBaseType_t n, UBaseType_t size, uint8_t *data, StaticQueue_t *q)
{
    *q = (StaticQueue_t){data, size, n, 0, 0};
    return q;
}
BaseType_t xQueueSend(QueueHandle_t q, const void *data, TickType_t wait)
{
    assert(wait == 0);
    if (q->count == q->capacity) {
        return pdFAIL;
    }
    memcpy(q->data + ((q->head + q->count) % q->capacity) * q->size, data, q->size);
    ++q->count;
    return pdPASS;
}
BaseType_t xQueueReceive(QueueHandle_t q, void *data, TickType_t wait)
{
    assert(wait == 0);
    if (!q->count) {
        return pdFAIL;
    }
    memcpy(data, q->data + q->head * q->size, q->size);
    q->head = (q->head + 1U) % q->capacity;
    --q->count;
    return pdPASS;
}
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t q)
{
    return q->count;
}
static uint32_t now;
static unsigned syncs, closes, writes;
static bool fail_sync;
static char output[8192];
static size_t used;
static sd_log_result_t ok(void *p)
{
    (void)p;
    assert(!critical);
    return SD_LOG_OK;
}
static sd_log_result_t create(void *p, const char *name)
{
    assert(name);
    return ok(p);
}
static sd_log_result_t write_file(void *p, const char *data, size_t size, size_t *written)
{
    (void)p;
    assert(!critical);
    ++writes;
    assert(used + size < sizeof(output));
    memcpy(output + used, data, size);
    used += size;
    output[used] = '\0';
    *written = size;
    return SD_LOG_OK;
}
static sd_log_result_t sync_file(void *p)
{
    (void)p;
    assert(!critical);
    ++syncs;
    return fail_sync ? SD_LOG_IO_ERROR : SD_LOG_OK;
}
static sd_log_result_t close_file(void *p)
{
    ++closes;
    return ok(p);
}
static uint32_t clock_ms(void *p)
{
    (void)p;
    return now;
}
int main(void)
{
    sd_log_storage_t storage = {NULL, ok, create, write_file, sync_file, close_file, clock_ms};
    hab_telemetry_record_t record = {0};
    record.packet_version = 1;
    assert(hab_sd_submit(&record) == SD_LOG_NOT_READY);
    assert(hab_sd_configure(&storage) == pdPASS);
    assert(hab_sd_start() == SD_LOG_OK);
    unsigned before = writes;
    for (unsigned i = 0; i < HAB_SD_QUEUE_LENGTH; ++i) {
        record.sequence_number = i;
        assert(hab_sd_submit(&record) == SD_LOG_OK);
    }
    record.sequence_number = 999;
    assert(hab_sd_submit(&record) == SD_LOG_FULL);
    assert(writes == before); /* Submission does no filesystem work. */
    for (unsigned i = 0; i < HAB_SD_QUEUE_LENGTH; ++i) {
        assert(hab_sd_service() == SD_LOG_OK);
    }
    assert(strstr(output, "1,1,0,0,"));
    assert(strstr(output, "1,1,31,0,"));
    assert(!strstr(output, "1,1,999,0,")); /* Queue copies immutable values. */
    hab_sd_status_t status;
    hab_sd_status(&status);
    assert(status.accepted == 32 && status.dropped == 1 && status.queue_high_water == 32);
    now = 1000;
    assert(hab_sd_service() == SD_LOG_OK && syncs == 2); /* Idle sync. */
    assert(hab_sd_submit(NULL) == SD_LOG_INVALID);
    record.mission_time_ms = now;
    assert(hab_sd_submit(&record) == SD_LOG_OK);
    assert(hab_sd_service() == SD_LOG_OK);
    now = 2000;
    fail_sync = true;
    assert(hab_sd_service() == SD_LOG_IO_ERROR && closes == 1);
    hab_sd_status(&status);
    assert(!status.ready && status.storage.synced_records == 32 && status.storage.dirty);
    assert(hab_sd_submit(&record) == SD_LOG_NOT_READY);
    assert(hab_sd_service() == SD_LOG_NOT_READY);
    assert(!critical);
    puts("PASS: copied zero-wait queue, overflow, idle sync, fault cleanup and snapshots");
    return 0;
}
