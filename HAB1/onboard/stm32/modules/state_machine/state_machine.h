#ifndef HAB_STATE_MACHINE_H
#define HAB_STATE_MACHINE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HAB_STATE_BOOT = 0,
    HAB_STATE_SELF_TEST,
    HAB_STATE_PREFLIGHT,
    HAB_STATE_ASCENT,
    HAB_STATE_DESCENT,
    HAB_STATE_LANDED,
    HAB_STATE_FAULT
} hab_state_t;

typedef struct {
    bool self_test_complete;
    bool systems_ready;
    bool launch_detected;
    bool descent_detected;
    bool landing_detected;
    bool critical_fault;
} hab_state_inputs_t;

typedef struct {
    hab_state_t current_state;
    uint32_t state_entered_ms;
} hab_state_machine_t;

void hab_state_machine_init(hab_state_machine_t *machine, uint32_t mission_time_ms);

void hab_state_machine_step(
    hab_state_machine_t *machine,
    const hab_state_inputs_t *inputs,
    uint32_t mission_time_ms);

#endif
