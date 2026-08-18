/*
 * HAB-001 integrated STM32 flight application skeleton.
 *
 * This is standard C with no dynamic allocation or RTOS dependency. Board and
 * device implementations remain TODO until the STM32 family and sensors are
 * selected.
 */

#include <stdbool.h>
#include <stdint.h>

#include "../../../../shared/telemetry/telemetry_record.h"
#include "../../modules/state_machine/state_machine.h"
#include "../../platform/board/board.h"

static hab_state_machine_t state_machine;
static hab_telemetry_record_t telemetry_record;

static void modules_init(void)
{
    /* TODO: initialize approved sensor, GPS, radio, power, and Pi-link modules. */
}

static void poll_subsystems(void)
{
    /* TODO: poll non-blocking subsystem APIs and preserve raw measurements. */
}

static hab_state_inputs_t collect_state_inputs(void)
{
    hab_state_inputs_t inputs = {0};

    /*
     * TODO: set these booleans from tested detector and health modules.
     * Thresholds do not belong in the state machine itself.
     */
    inputs.self_test_complete = false;
    inputs.systems_ready = false;
    inputs.launch_detected = false;
    inputs.descent_detected = false;
    inputs.landing_detected = false;
    inputs.critical_fault = false;

    return inputs;
}

static void build_telemetry_record(uint32_t mission_time_ms)
{
    telemetry_record.packet_version = HAB_TELEMETRY_PACKET_VERSION;
    telemetry_record.mission_time_ms = mission_time_ms;

    /* TODO: copy only validated measurements and set valid_fields bits. */
}

static void service_outputs(void)
{
    /* TODO: transmit radio telemetry and forward records to the Pi Zero. */
}

int main(void)
{
    board_init();
    modules_init();
    hab_state_machine_init(&state_machine, board_mission_time_ms());

    for (;;) {
        const uint32_t mission_time_ms = board_mission_time_ms();
        hab_state_inputs_t state_inputs;

        poll_subsystems();
        state_inputs = collect_state_inputs();
        hab_state_machine_step(&state_machine, &state_inputs, mission_time_ms);
        build_telemetry_record(mission_time_ms);
        service_outputs();
        board_watchdog_service();
    }
}
