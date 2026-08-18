/* Standalone FreeRTOS GPS bring-up application. */

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"

#include "../../../platform/board/board.h"

#define GPS_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE + 128U)

static StaticTask_t gps_task_control;
static StackType_t gps_task_stack[GPS_TASK_STACK_WORDS];

static void gps_task(void *context)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    for (;;) {
        /* TODO: poll the receiver and expose valid fix information. */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000U));
    }
}

int main(void)
{
    board_init();
    /* TODO: initialize the UART and reusable GPS module. */

    if (xTaskCreateStatic(
            gps_task,
            "gps",
            GPS_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1U,
            gps_task_stack,
            &gps_task_control) == NULL) {
        for (;;) {
        }
    }

    vTaskStartScheduler();

    for (;;) {
    }
}
