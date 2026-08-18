/* Standalone FreeRTOS telemetry bring-up application. */

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"

#include "../../../../../shared/telemetry/telemetry_record.h"
#include "../../../platform/board/board.h"

#define TELEMETRY_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE + 128U)

static StaticTask_t telemetry_task_control;
static StackType_t telemetry_task_stack[TELEMETRY_TASK_STACK_WORDS];

static void telemetry_task(void *context)
{
    hab_telemetry_record_t record = {0};
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    record.packet_version = HAB_TELEMETRY_PACKET_VERSION;

    for (;;) {
        record.mission_time_ms = board_mission_time_ms();
        /* TODO: encode and transmit only validated fields. */
        (void)record;
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000U));
    }
}

int main(void)
{
    board_init();

    if (xTaskCreateStatic(
            telemetry_task,
            "telemetry",
            TELEMETRY_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1U,
            telemetry_task_stack,
            &telemetry_task_control) == NULL) {
        for (;;) {
        }
    }

    vTaskStartScheduler();

    for (;;) {
    }
}
