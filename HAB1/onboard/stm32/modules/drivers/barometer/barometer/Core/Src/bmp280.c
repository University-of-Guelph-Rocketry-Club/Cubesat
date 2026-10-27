
#include "bmp280.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"
#include <stdint.h>
#include <stdio.h>
#include <main.h>


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

// read one or more consecutive registers from the BMP280.
static HAL_StatusTypeDef bmp280_read(bmp280_t *dev, uint8_t reg, uint8_t *data, uint16_t length) {
    // Bit 7 must be 1 to perform an SPI read. reg | 100000000
    uint8_t command = reg | 0x80;

    bmp280_CS_LOW();

    // sends bytes from STM32 memory out through the selected SPI peripheral
    // it sends the BMP280 register address we want to access
    HAL_StatusTypeDef status = HAL_SPI_Transmit(dev->spi, &command, 1, 100);

    if (status == HAL_OK)
    {
        // receives bytes from the SPI peripheral and stores them in the data buffer(data buffer is just a temporary holding area in RAM for bytes.)
        // length controls how many bytes are read from the BMP280
        status = HAL_SPI_Receive(dev->spi, data, length, 100);
    }

    bmp280_CS_HIGH();

    return status;
}

// Write one byte into a BMP280 register. 
static HAL_StatusTypeDef bmp280_write(
    bmp280_t *dev,
    uint8_t reg,
    uint8_t value
)
{
    // Bit 7 must be 0 to perform an SPI write. 
    uint8_t command = reg & 0x7F;

    bmp280_CS_LOW();

    // send the register address
    HAL_StatusTypeDef status = HAL_SPI_Transmit(dev->spi, &command, 1, 100);

    if (status == HAL_OK)
    {
        // Then send the value that should be written there.
        status = HAL_SPI_Transmit(dev->spi, &value, 1, 100);
    }

    bmp280_CS_HIGH();

    return status;
}


// read the sensor's factory calibration constants
// these are important for calculations since theyll be used to turn raw data 
// uinto accurate pressure valus (in pascals)
static HAL_StatusTypeDef bmp280_read_calibration(bmp280_t *dev)
{
    //create a 24 btye buffer in STM32 ram
    uint8_t data[BMP280_CALIB_LEN];

    HAL_StatusTypeDef status =
        bmp280_read(dev, BMP280_REG_CALIB_START, data, BMP280_CALIB_LEN);

    if (status != HAL_OK){
        return status;
    }

    // each calibration value is stored as two bytes, LSB first. 
    //takes two bytes and combines them into a 16 bit calibration number (bmp280 reg is 8 bits)
    //dig_ values are just calibration values from factory P for pressrue T for temp
    //data[1] bitsshifted over 8 or'ed with data[0] gets 16 bit value with the two concatendated
    dev->calib.dig_T1 =
        (uint16_t)((uint16_t)data[0] |
        ((uint16_t)data[1] << 8));

    dev->calib.dig_T2 =
        (int16_t)((uint16_t)data[2] |
        ((uint16_t)data[3] << 8));

    dev->calib.dig_T3 =
        (int16_t)((uint16_t)data[4] |
        ((uint16_t)data[5] << 8));


    dev->calib.dig_P1 =
        (uint16_t)((uint16_t)data[6] |
        ((uint16_t)data[7] << 8));

    dev->calib.dig_P2 =
        (int16_t)((uint16_t)data[8] |
        ((uint16_t)data[9] << 8));

    dev->calib.dig_P3 =
        (int16_t)((uint16_t)data[10] |
        ((uint16_t)data[11] << 8));

    dev->calib.dig_P4 =
        (int16_t)((uint16_t)data[12] |
        ((uint16_t)data[13] << 8));

    dev->calib.dig_P5 =
        (int16_t)((uint16_t)data[14] |
        ((uint16_t)data[15] << 8));

    dev->calib.dig_P6 =
        (int16_t)((uint16_t)data[16] |
        ((uint16_t)data[17] << 8));

    dev->calib.dig_P7 =
        (int16_t)((uint16_t)data[18] |
        ((uint16_t)data[19] << 8));

    dev->calib.dig_P8 =
        (int16_t)((uint16_t)data[20] |
        ((uint16_t)data[21] << 8));

    dev->calib.dig_P9 =
        (int16_t)((uint16_t)data[22] |
        ((uint16_t)data[23] << 8));

    return HAL_OK;
}


// verify, reset, calibrate, and start the BMP280. 
HAL_StatusTypeDef bmp280_init(bmp280_t *dev)
{
    uint8_t chip_id;

    if (dev == NULL || dev->spi == NULL){
        return HAL_ERROR;
    }

    /* Read the chip ID to make sure the BMP280 responds. */
    HAL_StatusTypeDef status = bmp280_read(dev, BMP280_REG_ID, &chip_id, 1);

    if (status != HAL_OK){
        return status;
    }

    if (chip_id != BMP280_EXPECTED_CHIP_ID){
        return HAL_ERROR;
    }


    // Reset the BMP280 so it starts from a known state. 
    status = bmp280_write(dev, BMP280_REG_RESET, BMP280_RESET_CMD);

    if (status != HAL_OK){
        return status;
    }

    // Give the sensor time to reload its calibration constants. 
    HAL_Delay(5);


    //store the sensor-specific calibration constants. 
    status = bmp280_read_calibration(dev);

    if (status != HAL_OK){
        return status;
    }


    // Enable x1 temperature, x1 pressure and normal mode.
    status = bmp280_write(dev,BMP280_REG_CTRL_MEAS, BMP280_CTRL_MEAS_VALUE);

    return status;
}


// read and compensate the pressure measurement
/*
-goes from raw temp
-temp calibration curve with T-dig
-t_fine (temp affects pressure)
-raw presre + temp info + dig_p1..
-pressure in Pa
*/
HAL_StatusTypeDef bmp280_read_pressure(bmp280_t *dev, float *pressure_pa, float *temperature_c){
    uint8_t data[6];

    if (dev == NULL || pressure_pa == NULL || temperature_c == NULL){
        return HAL_ERROR;
    }


    // read the 3 pressure bytes and 3 temperature bytes together
    HAL_StatusTypeDef status = bmp280_read(dev, BMP280_REG_DATA_START, data, 6);

    if (status != HAL_OK){
        return status;
    }


    // combine the three pressure bytes into one 20 bit value
    int32_t adc_P =
        ((int32_t)data[0] << 12) |
        ((int32_t)data[1] << 4)  |
        ((int32_t)data[2] >> 4);

    int32_t adc_T =
        ((int32_t)data[3] << 12) |
        ((int32_t)data[4] << 4)  |
        ((int32_t)data[5] >> 4);


    // calculate t_fine(temp value used by bmp280 comp formulas), required by the pressure equation
    double var1 =
        ((double)adc_T / 16384.0 -
        (double)dev->calib.dig_T1 / 1024.0) *
        (double)dev->calib.dig_T2;

    double var2 =
        (((double)adc_T / 131072.0 -
        (double)dev->calib.dig_T1 / 8192.0) *

        ((double)adc_T / 131072.0 -
        (double)dev->calib.dig_T1 / 8192.0)) *

        (double)dev->calib.dig_T3;

    double t_fine = var1 + var2;

    // Convert t_fine into temperature in degrees Celsius
    *temperature_c = (float)(t_fine / 5120.0);


    // use t_fine and calibration constants to correct pressure. 
    var1 = (t_fine / 2.0) - 64000.0;

    var2 =
        var1 * var1 *
        (double)dev->calib.dig_P6 /
        32768.0;

    var2 =
        var2 +
        var1 *
        (double)dev->calib.dig_P5 *
        2.0;

    var2 =
        (var2 / 4.0) +
        ((double)dev->calib.dig_P4 * 65536.0);

    var1 =
        ((double)dev->calib.dig_P3 *
        var1 * var1 /
        524288.0 +

        (double)dev->calib.dig_P2 *
        var1) /
        524288.0;

    var1 =
        (1.0 + var1 / 32768.0) *
        (double)dev->calib.dig_P1;


    // A zero value here would cause division by zero. 
    if (var1 == 0.0){
        return HAL_ERROR;
    }


    double pressure =
        1048576.0 -
        (double)adc_P;

    pressure =
        (pressure - var2 / 4096.0) *
        6250.0 /
        var1;

    var1 =
        (double)dev->calib.dig_P9 *
        pressure *
        pressure /
        2147483648.0;

    var2 =
        pressure *
        (double)dev->calib.dig_P8 /
        32768.0;

    pressure =
        pressure +
        (var1 + var2 +
        (double)dev->calib.dig_P7) /
        16.0;


    // tore the final pressure measurement in Pascalss
    *pressure_pa = (float)pressure;

    return HAL_OK;
}
