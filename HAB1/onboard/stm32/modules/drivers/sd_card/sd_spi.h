#ifndef HAB_SD_SPI_H
#define HAB_SD_SPI_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Mode 0, MSB first, dedicated bus. Every callback must return within a bounded
 * time. now_ms must advance during polling. Select true asserts CS low.
 * transfer transmits AND receives length bytes (buffers may overlap).
 */
typedef struct {
    void *context;
    bool (*transfer)(void *, const uint8_t *, uint8_t *, size_t);
    void (*select)(void *, bool);
    bool (*set_fast)(void *, bool); /* false: 100-400 kHz; true: board run rate */
    uint32_t (*now_ms)(void *);
    void (*delay_ms)(void *, uint32_t);
} sd_spi_bus_t;
typedef struct {
    sd_spi_bus_t bus;
    bool initialized, block_addressed, io_failed;
    uint8_t last_command, last_response;
} sd_spi_t;
bool sd_spi_init(sd_spi_t *card, const sd_spi_bus_t *bus);
bool sd_spi_read(sd_spi_t *card, uint32_t sector, uint8_t *data);
bool sd_spi_write(sd_spi_t *card, uint32_t sector, const uint8_t *data);
bool sd_spi_sync(sd_spi_t *card);
/* Single card, FatFs physical drive 0, one owning task. Bind before mounting. */
void sd_spi_bind(sd_spi_t *card, const sd_spi_bus_t *bus);
#endif
