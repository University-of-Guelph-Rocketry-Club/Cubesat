/* Standalone FreeRTOS STM32-to-Pi link bring-up application. */

#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"

#include "../../../platform/board/board.h"

#define PI_LINK_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE + 128U)

static StaticTask_t pi_link_task_control;
static StackType_t pi_link_task_stack[PI_LINK_TASK_STACK_WORDS];

static void pi_link_task(void *context)
{
    (void)context;

    for (;;) {
        /* TODO: send bounded test frames and block on queues/notifications. */
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
}

int main(void)
{
    board_init();
    /* TODO: initialize the reusable Pi-link transport module. */

    if (xTaskCreateStatic(
            pi_link_task,
            "pi-link",
            PI_LINK_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1U,
            pi_link_task_stack,
            &pi_link_task_control) == NULL) {
        for (;;) {
        }
    }

    vTaskStartScheduler();

    for (;;) {
    }
}
