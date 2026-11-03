#ifndef HAB_BOARD_SD_H
#define HAB_BOARD_SD_H
#include "stm32f4xx_hal.h"
#include "../../modules/sd_logger/sd_logger.h"
/* Call once in board_init before creating tasks. The board owns GPIO/SPI setup,
 * power stabilization, a dedicated bus, and 100-400kHz init/run prescalers. */
void board_sd_configure(SPI_HandleTypeDef *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                        uint32_t init_prescaler, uint32_t run_prescaler);
sd_log_storage_t board_sd_storage(void);
#endif
