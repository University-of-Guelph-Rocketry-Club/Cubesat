
#include "bmp280.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"
#include <stdint.h>
#include <stdio.h>


/*
static: only this file can use this function
inline: function is small enough to just paste its code (HAL_GPIO...) when its called

*/
static inline void bmp280_CS_LOW(void)
{
    //GPIO_TypeDef *GPIOx = which gpio port is the pin on (#define CSB_GPIO_Port GPIOA)
    //uint16_t GPIO_Pin = which pin inside that port (#define CSB_Pin GPIO_PIN_4)
    //
    HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_RESET);
}

static inline void bmp280_CS_HIGH(void)
{
    HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_SET);
}

//dev stands for device(pointer to device)
HAL_StatusTypeDef bmp280_init(bmp280_t *dev){

};

HAL_StatusTypeDef bmp280_read_raw(bmp280_t *dev, bmp280_raw_t *raw){

};

void bmp280_convert(const bmp280_t *dev, const bmp280_raw_t *raw, bmp280_sample_t *out){

};

sensor_status_t bmp280_reset(bmp280_t *dev){

};
