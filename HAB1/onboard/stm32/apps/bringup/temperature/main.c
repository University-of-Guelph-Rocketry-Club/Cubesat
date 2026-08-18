/* Standalone FreeRTOS temperature-sensor bring-up application. */

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"

#include "../../../platform/board/board.h"

#define TEMPERATURE_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE + 128U)

static StaticTask_t temperature_task_control;
static StackType_t temperature_task_stack[TEMPERATURE_TASK_STACK_WORDS];

static void temperature_task(void *context)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    for (;;) {
        /* TODO: poll, validate, and expose temperature_c. */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000U));
    }
}

int main(void)
{
    board_init();
    /* TODO: initialize the reusable temperature module. */

    if (xTaskCreateStatic(
            temperature_task,
            "temperature",
            TEMPERATURE_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1U,
            temperature_task_stack,
            &temperature_task_control) == NULL) {
        for (;;) {
        }
    }

    vTaskStartScheduler();

    for (;;) {
    }
}
