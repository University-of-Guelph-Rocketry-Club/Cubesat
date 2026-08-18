#include "flight_tasks.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "task.h"

#include "../../../../shared/telemetry/telemetry_record.h"
#include "../../modules/state_machine/state_machine.h"
#include "../../platform/board/board.h"

/* Provisional values: measure and review on the selected STM32 target. */
#define SENSOR_TASK_STACK_WORDS    (configMINIMAL_STACK_SIZE + 128U)
#define GPS_TASK_STACK_WORDS       (configMINIMAL_STACK_SIZE + 128U)
#define STATE_TASK_STACK_WORDS     (configMINIMAL_STACK_SIZE + 128U)
#define TELEMETRY_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE + 128U)
#define PI_LINK_TASK_STACK_WORDS   (configMINIMAL_STACK_SIZE + 128U)
#define HEALTH_TASK_STACK_WORDS    (configMINIMAL_STACK_SIZE + 128U)

static StaticTask_t sensor_task_control;
static StaticTask_t gps_task_control;
static StaticTask_t state_task_control;
static StaticTask_t telemetry_task_control;
static StaticTask_t pi_link_task_control;
static StaticTask_t health_task_control;

static StackType_t sensor_task_stack[SENSOR_TASK_STACK_WORDS];
static StackType_t gps_task_stack[GPS_TASK_STACK_WORDS];
static StackType_t state_task_stack[STATE_TASK_STACK_WORDS];
static StackType_t telemetry_task_stack[TELEMETRY_TASK_STACK_WORDS];
static StackType_t pi_link_task_stack[PI_LINK_TASK_STACK_WORDS];
static StackType_t health_task_stack[HEALTH_TASK_STACK_WORDS];

static void sensor_task(void *context)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    for (;;) {
        /* TODO: poll sensors and publish an immutable validated snapshot. */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000U));
    }
}

static void gps_task(void *context)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    for (;;) {
        /* TODO: parse GPS input and publish fix/status snapshots. */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000U));
    }
}

static void state_task(void *context)
{
    hab_state_machine_t machine;
    hab_state_inputs_t inputs = {0};
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    hab_state_machine_init(&machine, board_mission_time_ms());

    for (;;) {
        /*
         * TODO: receive detector and health inputs through bounded RTOS
         * objects. This task is the sole owner of machine.
         */
        hab_state_machine_step(&machine, &inputs, board_mission_time_ms());
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100U));
    }
}

static void telemetry_task(void *context)
{
    hab_telemetry_record_t record = {0};
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    record.packet_version = HAB_TELEMETRY_PACKET_VERSION;

    for (;;) {
        record.mission_time_ms = board_mission_time_ms();
        /* TODO: consume snapshots, set validity flags, encode, and transmit. */
        (void)record;
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(5000U));
    }
}

static void pi_link_task(void *context)
{
    (void)context;

    for (;;) {
        /* TODO: block on a bounded queue and send without stalling producers. */
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
}

static void health_task(void *context)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    (void)context;

    for (;;) {
        bool required_tasks_healthy = false;

        /* TODO: derive this value from required task heartbeat/event bits. */
        if (required_tasks_healthy) {
            board_watchdog_service();
        }
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(500U));
    }
}

BaseType_t flight_tasks_create(void)
{
    if (xTaskCreateStatic(
            sensor_task,
            "sensors",
            SENSOR_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 2U,
            sensor_task_stack,
            &sensor_task_control) == NULL) {
        return pdFAIL;
    }

    if (xTaskCreateStatic(
            gps_task,
            "gps",
            GPS_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 2U,
            gps_task_stack,
            &gps_task_control) == NULL) {
        return pdFAIL;
    }

    if (xTaskCreateStatic(
            state_task,
            "state",
            STATE_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 3U,
            state_task_stack,
            &state_task_control) == NULL) {
        return pdFAIL;
    }

    if (xTaskCreateStatic(
            telemetry_task,
            "telemetry",
            TELEMETRY_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1U,
            telemetry_task_stack,
            &telemetry_task_control) == NULL) {
        return pdFAIL;
    }

    if (xTaskCreateStatic(
            pi_link_task,
            "pi-link",
            PI_LINK_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1U,
            pi_link_task_stack,
            &pi_link_task_control) == NULL) {
        return pdFAIL;
    }

    if (xTaskCreateStatic(
            health_task,
            "health",
            HEALTH_TASK_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 3U,
            health_task_stack,
            &health_task_control) == NULL) {
        return pdFAIL;
    }

    return pdPASS;
}
