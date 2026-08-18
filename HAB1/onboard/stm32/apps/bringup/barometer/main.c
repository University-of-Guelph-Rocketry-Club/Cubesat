/* Standalone FreeRTOS barometer bring-up application. */

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"

#include "../../../platform/board/board.h"

#define BAROMETER_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE + 128U)

static StaticTask_t barometer_task_control;
static StackType_t barometer_task_stack[BAROMETER_TASK_STACK_WORDS];

static void barometer_task(void *context)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    for (;;) {
        /* TODO: poll, validate, and expose pressure_pa. */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000U));
    }
}

int main(void)
{
    board_init();
    /* TODO: initialize the reusable barometer module. */

    if (xTaskCreateStatic(
            barometer_task,
            "barometer",
            BAROMETER_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1U,
            barometer_task_stack,
            &barometer_task_control) == NULL) {
        for (;;) {
        }
    }

    vTaskStartScheduler();

    for (;;) {
    }
}
