/*
 * HAB-001 integrated STM32 flight application skeleton.
 *
 * This is the FreeRTOS application entry point. Board, kernel-port, and device
 * implementations remain TODO until the STM32 family and sensors are selected.
 */

#include "FreeRTOS.h"
#include "task.h"

#include "../../platform/board/board.h"
#include "flight_tasks.h"

static void modules_init(void)
{
    /* TODO: initialize approved sensor, GPS, radio, power, and Pi-link modules. */
}

int main(void)
{
    board_init();
    modules_init();

    if (flight_tasks_create() != pdPASS) {
        /* TODO: expose task-creation failure through an approved board path. */
        for (;;) {
        }
    }

    vTaskStartScheduler();

    /* Reaching this point means the scheduler could not start. */
    for (;;) {
    }
}
