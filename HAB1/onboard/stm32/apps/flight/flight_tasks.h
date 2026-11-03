#ifndef HAB_FLIGHT_TASKS_H
#define HAB_FLIGHT_TASKS_H

#include "FreeRTOS.h"
#include "../../../../shared/telemetry/telemetry_record.h"

BaseType_t flight_tasks_create(void);
/* Call from the real record assembler at logging rate, not radio rate.
 * pdPASS means queued, not yet durable. pdFAIL means disabled/unavailable/full
 * or invalid; inspect hab_sd_status when logging is enabled. */
BaseType_t flight_log_record(const hab_telemetry_record_t *record);

#endif
