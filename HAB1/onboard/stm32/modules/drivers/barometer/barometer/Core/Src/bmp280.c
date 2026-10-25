#include <stdio.h>
#include "bmp280.h"
#include "stm32f4xx_hal.h"


//keeps data clean and avoids having other functions/programs touch data
typedef struct {
    SPI_HandleTypeDef   *spi;
    GPIO_TypeDef        *cs_port;
    uint16_t             cs_pin;
    bmp280_calib_t       calib;
    bmp280_config_t      config;
    uint32_t             err_count;
} bmp280_t;

sensor_status_t bmp280_init(bmp280_t *dev){

};

sensor_status_t bmp280_read_raw(bmp280_t *dev, bmp280_raw_t *raw){

};

void bmp280_convert(const bmp280_t *dev, const bmp280_raw_t *raw, bmp280_sample_t *out){

};

sensor_status_t bmp280_reset(bmp280_t *dev){

};
