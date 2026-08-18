#ifndef HAB_BOARD_H
#define HAB_BOARD_H

#include <stdint.h>

void board_init(void);
uint32_t board_mission_time_ms(void);
void board_watchdog_service(void);

#endif
