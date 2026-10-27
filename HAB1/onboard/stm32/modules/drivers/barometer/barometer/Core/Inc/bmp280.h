#ifndef BMP280_H
#define BMP280_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

//contains a fixed id number programmed by bosch used to confirm SPI communication
#define BMP280_REG_ID           0xD0 
//reading reg id should return the following
#define BMP280_EXPECTED_CHIP_ID 0x58
//writing a special value to this register causes the bmp to software reset
#define BMP280_REG_RESET        0xE0
//reset special value
#define BMP280_RESET_CMD        0xB6 
//controls pressure/temperature oversampling and sensor operating mode
#define BMP280_REG_CTRL_MEAS       0xF4
//pressure raw data starts here
#define BMP280_REG_DATA_START      0xF7
//sets over sampping sampling to 1x for temp and pressure and normal operating mode
#define BMP280_CTRL_MEAS_VALUE     0x27

// factory calibration constants start here and use 24 bytes
#define BMP280_REG_CALIB_START    0x88
#define BMP280_CALIB_LEN          24

//calibration constants for this specific bmp 280
typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

} bmp280_calib_t;

// Contains the STM32 hardware and calibration data used by the sensor. 
typedef struct
{
    SPI_HandleTypeDef *spi;

    bmp280_calib_t calib;

} bmp280_t;

// holds the SPI peripheral and calibration data for the sensor
typedef struct
{
    SPI_HandleTypeDef *spi;
    bmp280_calib_t calib;

} bmp280_t;


// Sets up the BMP280 and reads its calibration constants.
HAL_StatusTypeDef bmp280_init(bmp280_t *dev);


// Reads the sensor and returns compensated pressure in pascals
HAL_StatusTypeDef bmp280_read_pressure(
    bmp280_t *dev,
    float *pressure_pa,
    float *temperature_c
);

#endif // BMP280_H