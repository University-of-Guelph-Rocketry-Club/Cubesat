#ifndef HAB_FLIGHT_SD_LOGGING_H
#define HAB_FLIGHT_SD_LOGGING_H
#include "../../modules/sd_logger/sd_logger.h"
#include "FreeRTOS.h"

#define HAB_SD_QUEUE_LENGTH 32U
typedef struct {
    sd_log_status_t storage;
    /* Accepted = copied to queue; dropped = unavailable/full. Invalid includes
     * input rejected at submission or later during owner-task serialization. */
    uint32_t accepted, dropped, invalid;
    UBaseType_t queue_high_water;
    bool ready;
} hab_sd_status_t;
/* Call configure once before producers/task start. No live reconfiguration. */
BaseType_t hab_sd_configure(const sd_log_storage_t *storage);
/* Logger task only: mount/create, then service at least every 100ms. */
sd_log_result_t hab_sd_start(void);
sd_log_result_t hab_sd_service(void);
/* Producer task only: zero-wait copied submission, never touches the card. */
sd_log_result_t hab_sd_submit(const hab_telemetry_record_t *record);
/* Critical-section protected copy; no card I/O. Not an ISR API. */
void hab_sd_status(hab_sd_status_t *status);
#endif
