/* Hardware transport only. The generic SD driver owns command sequencing. */
#include "sd_spi_hal.h"
static bool transfer(void *context, const uint8_t *transmit, uint8_t *receive, size_t size)
{
    /* SD reads still transmit dummy clocks; always use full-duplex transfer.
     * The finite HAL timeout prevents a peripheral failure blocking indefinitely. */
    sd_spi_hal_t *port = context;
    if (size > UINT16_MAX) {
        return false;
    }
    return HAL_SPI_TransmitReceive(port->spi, (uint8_t *)transmit, receive, (uint16_t)size, 100) ==
           HAL_OK;
}
static void select_card(void *context, bool selected)
{
    sd_spi_hal_t *port = context;
    HAL_GPIO_WritePin(port->cs_port, port->cs_pin, selected ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
static bool set_speed(void *context, bool fast)
{
    /* Prescalers come from the board, because SPI source clocks vary by target. */
    sd_spi_hal_t *port = context;
    port->spi->Init.BaudRatePrescaler = fast ? port->run_prescaler : port->init_prescaler;
    return HAL_SPI_Init(port->spi) == HAL_OK;
}
static uint32_t clock_ms(void *context)
{
    (void)context;
    return HAL_GetTick();
}
static void delay_ms(void *context, uint32_t ms)
{
    (void)context;
    HAL_Delay(ms);
}
sd_spi_bus_t sd_spi_hal_bus(sd_spi_hal_t *port)
{
    sd_spi_bus_t bus = {port, transfer, select_card, set_speed, clock_ms, delay_ms};
    return bus;
}
