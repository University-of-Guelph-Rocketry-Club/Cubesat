#ifndef HAB_SD_LOGGER_H
#define HAB_SD_LOGGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../../../shared/telemetry/telemetry_record.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_LOGGER_ROW_CAPACITY 512U
#define SD_LOGGER_SYNC_INTERVAL_MS 1000U
#define SD_LOGGER_LOSS_BUDGET_MS 5000U

typedef enum {
    SD_LOG_OK = 0,
    SD_LOG_EXISTS,
    SD_LOG_FULL,
    SD_LOG_IO_ERROR,
    SD_LOG_INVALID,
    SD_LOG_NOT_READY,
    SD_LOG_NAMES_EXHAUSTED
} sd_log_result_t;

typedef enum { SD_LOG_STOPPED, SD_LOG_READY, SD_LOG_FAULT } sd_log_state_t;

/* All callbacks are synchronous and must have bounded device timeouts.
 * create_new MUST exclusively create, never truncate/append an existing file.
 * sync must commit file metadata and wait for block-device completion.
 * close releases the handle even after a failed write/sync. It may return error.
 * now_ms uses the same wrapping monotonic timebase as record.mission_time_ms.
 */
typedef struct {
    void *context;
    sd_log_result_t (*mount)(void *context);
    sd_log_result_t (*create_new)(void *context, const char *name);
    sd_log_result_t (*write)(void *context, const char *data, size_t length, size_t *written);
    sd_log_result_t (*sync)(void *context);
    sd_log_result_t (*close)(void *context);
    uint32_t (*now_ms)(void *context);
} sd_log_storage_t;

typedef struct {
    sd_log_state_t state;
    sd_log_result_t last_error;
    /* Session counters: written does not imply successfully synced. */
    uint32_t written_records;
    uint32_t synced_records;
    uint32_t last_sync_ms;
    uint32_t last_synced_sequence;
    uint32_t last_synced_mission_time_ms;
    uint32_t oldest_pending_age_ms;
    uint32_t max_age_at_sync_ms;
    uint32_t missed_durability_deadlines;
    /* Sequence zero is valid; use this flag rather than testing sequence != 0. */
    bool has_synced_record;
    bool dirty;
    char filename[13];
} sd_log_status_t;

typedef struct {
    sd_log_storage_t storage;
    sd_log_status_t status;
    uint32_t pending_oldest_ms;
    uint32_t pending_sequence;
    uint32_t pending_mission_time_ms;
    bool opened;
    char row[SD_LOGGER_ROW_CAPACITY];
} sd_logger_t;

/* No explicit module heap allocation or private tasks (libc formatting may allocate).
 * One task owns every call, including
 * status access; application must synchronize snapshots for other tasks.
 * init is for a fresh/stopped object only; never discard an open handle.
 */
sd_log_result_t sd_logger_init(sd_logger_t *logger, const sd_log_storage_t *storage);
/* Mount, exclusively create a new session, and sync its header before READY. */
sd_log_result_t sd_logger_start(sd_logger_t *logger);
/* Reject invalid input before I/O; append one row and service due sync. */
sd_log_result_t sd_logger_append(sd_logger_t *logger, const hab_telemetry_record_t *record);
/* Call at least once per application service cycle, even with an idle queue. */
sd_log_result_t sd_logger_poll(sd_logger_t *logger);
/* Failure preserves the previous confirmed boundary and dirty pending state. */
sd_log_result_t sd_logger_sync(sd_logger_t *logger);
/* Sync if healthy, then release the handle while preserving any original fault. */
sd_log_result_t sd_logger_stop(sd_logger_t *logger);
/* Owner-task snapshot; the flight wrapper protects copies for other tasks. */
sd_log_result_t sd_logger_get_status(sd_logger_t *logger, sd_log_status_t *status);

#ifdef __cplusplus
}
#endif
#endif
