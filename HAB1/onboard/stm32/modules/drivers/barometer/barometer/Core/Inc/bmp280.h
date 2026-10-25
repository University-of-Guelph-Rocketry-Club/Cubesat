#include "main.h"
#ifndef BMP280_H
#define BMP280_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

//contains a fixed id number programmed by bosch
#define BMP280_REG_ID           0xD0 
//reading reg id should return the following
#define bmp280_EXPECTED_CHIP_ID 0x58
//writing a special value to this register causes the bmp to software reset
#define bmp280_REG_RESET        0xE0
//reset special value
#define bmp280_RESET_CMD        0xB6 
//tells us what the bmp280 is doing internally 
#define BMP280_REG_STATUS        0xF3
#define bmp280_REG_CALIB_START  0x88 //addresses 0xA1...0x88
#define bmp280_CALIB_LEN        24

//keeps data clean and avoids having other functions/programs touch data
typedef struct {
    SPI_HandleTypeDef   *spi;

    GPIO_TypeDef        *cs_port;
    uint16             cs_pin;
    
    bmp280_calib      calib;
    bmp280_config      config;
    uint32             err_count;
} bmp280;

int bmp280_Initialize();

sensor_status bmp280_read_raw(
    bmp280 *dev,
    bmp280_raw *raw
);

void bmp280_convert(
    const bmp280 *dev,
    const bmp280_raw *raw,
    bmp280_sample *out
);

sensor_status bmp280_reset(bmp280 *dev);
