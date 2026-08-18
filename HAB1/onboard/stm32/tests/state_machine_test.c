#include <assert.h>

#include "../modules/state_machine/state_machine.h"

int main(void)
{
    hab_state_machine_t machine;
    hab_state_inputs_t inputs = {0};

    hab_state_machine_init(&machine, 0U);
    assert(machine.current_state == HAB_STATE_BOOT);

    hab_state_machine_step(&machine, &inputs, 1U);
    assert(machine.current_state == HAB_STATE_SELF_TEST);

    inputs.self_test_complete = true;
    inputs.systems_ready = true;
    hab_state_machine_step(&machine, &inputs, 2U);
    assert(machine.current_state == HAB_STATE_PREFLIGHT);

    inputs.launch_detected = true;
    hab_state_machine_step(&machine, &inputs, 3U);
    assert(machine.current_state == HAB_STATE_ASCENT);

    inputs.descent_detected = true;
    hab_state_machine_step(&machine, &inputs, 4U);
    assert(machine.current_state == HAB_STATE_DESCENT);

    inputs.landing_detected = true;
    hab_state_machine_step(&machine, &inputs, 5U);
    assert(machine.current_state == HAB_STATE_LANDED);

    inputs.critical_fault = true;
    hab_state_machine_step(&machine, &inputs, 6U);
    assert(machine.current_state == HAB_STATE_FAULT);

    return 0;
}
