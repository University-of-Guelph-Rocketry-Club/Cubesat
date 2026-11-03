#ifndef HAB_SD_SPI_HAL_H
#define HAB_SD_SPI_HAL_H
#include "stm32f4xx_hal.h"
#include "sd_spi.h"
typedef struct {
    SPI_HandleTypeDef *spi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint32_t init_prescaler, run_prescaler;
} sd_spi_hal_t;
sd_spi_bus_t sd_spi_hal_bus(sd_spi_hal_t *port);
#endif
