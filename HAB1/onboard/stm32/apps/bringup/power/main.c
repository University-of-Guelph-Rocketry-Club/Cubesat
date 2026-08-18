/* Standalone FreeRTOS power-monitor bring-up application. */

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"

#include "../../../platform/board/board.h"

#define POWER_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE + 128U)

static StaticTask_t power_task_control;
static StackType_t power_task_stack[POWER_TASK_STACK_WORDS];

static void power_task(void *context)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    for (;;) {
        /* TODO: poll, validate, and expose battery_voltage_v. */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000U));
    }
}

int main(void)
{
    board_init();
    /* TODO: initialize the board ADC and reusable power module. */

    if (xTaskCreateStatic(
            power_task,
            "power",
            POWER_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1U,
            power_task_stack,
            &power_task_control) == NULL) {
        for (;;) {
        }
    }

    vTaskStartScheduler();

    for (;;) {
    }
}
