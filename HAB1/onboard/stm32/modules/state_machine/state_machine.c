#include "state_machine.h"

#include <stddef.h>

static void enter_state(
    hab_state_machine_t *machine,
    hab_state_t next_state,
    uint32_t mission_time_ms)
{
    machine->current_state = next_state;
    machine->state_entered_ms = mission_time_ms;
}

void hab_state_machine_init(hab_state_machine_t *machine, uint32_t mission_time_ms)
{
    if (machine == NULL) {
        return;
    }

    enter_state(machine, HAB_STATE_BOOT, mission_time_ms);
}

void hab_state_machine_step(
    hab_state_machine_t *machine,
    const hab_state_inputs_t *inputs,
    uint32_t mission_time_ms)
{
    if ((machine == NULL) || (inputs == NULL)) {
        return;
    }

    if (inputs->critical_fault) {
        enter_state(machine, HAB_STATE_FAULT, mission_time_ms);
        return;
    }

    switch (machine->current_state) {
    case HAB_STATE_BOOT:
        enter_state(machine, HAB_STATE_SELF_TEST, mission_time_ms);
        break;

    case HAB_STATE_SELF_TEST:
        if (inputs->self_test_complete && inputs->systems_ready) {
            enter_state(machine, HAB_STATE_PREFLIGHT, mission_time_ms);
        }
        break;

    case HAB_STATE_PREFLIGHT:
        if (inputs->launch_detected) {
            enter_state(machine, HAB_STATE_ASCENT, mission_time_ms);
        }
        break;

    case HAB_STATE_ASCENT:
        if (inputs->descent_detected) {
            enter_state(machine, HAB_STATE_DESCENT, mission_time_ms);
        }
        break;

    case HAB_STATE_DESCENT:
        if (inputs->landing_detected) {
            enter_state(machine, HAB_STATE_LANDED, mission_time_ms);
        }
        break;

    case HAB_STATE_LANDED:
    case HAB_STATE_FAULT:
        break;

    default:
        enter_state(machine, HAB_STATE_FAULT, mission_time_ms);
        break;
    }
}
