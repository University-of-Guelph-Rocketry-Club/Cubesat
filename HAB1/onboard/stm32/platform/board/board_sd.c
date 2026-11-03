/* Bridge the selected flight-board SPI/pins into the reusable logger stack.
 * Configuration happens once during board_init, before any logging task runs.
 */
#include "board_sd.h"
#include "board.h"
#include "../../modules/drivers/sd_card/sd_spi_hal.h"
#include "../../modules/sd_logger/sd_fatfs.h"
static sd_spi_hal_t port;
static sd_spi_t card;
static sd_fatfs_t filesystem;
static bool configured;
/* Durability ages must use the same timebase as flight-record timestamps. */
static uint32_t clock_ms(void *unused)
{
    (void)unused;
    return board_mission_time_ms();
}
void board_sd_configure(SPI_HandleTypeDef *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                        uint32_t init_prescaler, uint32_t run_prescaler)
{
    port = (sd_spi_hal_t){spi, cs_port, cs_pin, init_prescaler, run_prescaler};
    configured = spi != NULL && cs_port != NULL && cs_pin != 0;
    if (configured) {
        sd_spi_bus_t bus = sd_spi_hal_bus(&port);
        sd_spi_bind(&card, &bus);
    }
}
sd_log_storage_t board_sd_storage(void)
{
    /* An unconfigured board returns invalid callbacks, causing explicit startup
     * failure rather than silently using the Nucleo's test pin assignments. */
    if (!configured) {
        sd_log_storage_t unavailable = {0};
        return unavailable;
    }
    return sd_fatfs_storage(&filesystem, clock_ms, NULL);
}
