/* SPI block-device layer: SD command framing, startup, and 512-byte I/O.
 * One task owns this card and FatFs drive 0; board details stay in bus callbacks.
 */
#include "sd_spi.h"
#include "ff.h"
#include "diskio.h"
#include <string.h>

/* SD SPI command numbers. Application commands require CMD55 first. */
enum {
    SD_CMD_GO_IDLE = 0,
    SD_CMD_SEND_IF_COND = 8,
    SD_CMD_SEND_STATUS = 13,
    SD_CMD_SET_BLOCKLEN = 16,
    SD_CMD_READ_SINGLE_BLOCK = 17,
    SD_CMD_WRITE_SINGLE_BLOCK = 24,
    SD_ACMD_SEND_OP_COND = 41,
    SD_CMD_APP_CMD = 55,
    SD_CMD_READ_OCR = 58
};

static sd_spi_t *bound_card;
static sd_spi_bus_t bound_bus;

static uint8_t exchange(sd_spi_t *card, uint8_t outgoing)
{
    uint8_t incoming = 0xff;
    if (!card->bus.transfer(card->bus.context, &outgoing, &incoming, 1)) {
        card->io_failed = true;
    }
    return incoming;
}
static void deselect_card(sd_spi_t *card)
{
    card->bus.select(card->bus.context, false);
    (void)exchange(card, 0xff); /* Trailing clocks let the card release its DO line. */
}
/* Poll until the card releases MISO. Both elapsed time and loop count are bounded. */
static bool wait_ready(sd_spi_t *card, uint32_t timeout_ms)
{
    uint32_t start = card->bus.now_ms(card->bus.context);
    for (uint32_t tries = 0; tries < 1000000U; ++tries) {
        uint8_t value = exchange(card, 0xff);
        if (card->io_failed) {
            return false;
        }
        if (value == 0xff) {
            return true;
        }
        if ((uint32_t)(card->bus.now_ms(card->bus.context) - start) >= timeout_ms) {
            break;
        }
    }
    return false;
}
/* Keep CS asserted through response/data; the caller ends the transaction. */
static uint8_t send_command(sd_spi_t *card, uint8_t command_index, uint32_t argument)
{
    deselect_card(card);
    card->bus.select(card->bus.context, true);
    card->last_command = command_index;
    /* CMD0 must be issuable during recovery, even when DO is held busy. */
    if (command_index != SD_CMD_GO_IDLE && !wait_ready(card, 1000)) {
        return card->last_response = 0xff;
    }
    uint8_t tx_bytes[6] = {(uint8_t)(0x40U | command_index),
                           (uint8_t)(argument >> 24),
                           (uint8_t)(argument >> 16),
                           (uint8_t)(argument >> 8),
                           (uint8_t)argument,
                           command_index == SD_CMD_GO_IDLE
                               ? 0x95
                               : (command_index == SD_CMD_SEND_IF_COND ? 0x87 : 0x01)};
    uint8_t rx_bytes[6];
    if (!card->bus.transfer(card->bus.context, tx_bytes, rx_bytes, sizeof(tx_bytes))) {
        card->io_failed = true;
    }
    uint8_t result = 0xff;
    for (unsigned i = 0; i < 10 && !card->io_failed; ++i) {
        result = exchange(card, 0xff);
        if (!(result & 0x80U)) {
            break;
        }
    }
    card->last_response = result;
    return result;
}
/* Wait for a start token, receive payload, then consume the two CRC bytes. */
static bool receive_data(sd_spi_t *card, uint8_t *data, size_t size)
{
    uint32_t start = card->bus.now_ms(card->bus.context);
    uint8_t token = 0xff;
    for (uint32_t tries = 0; tries < 1000000U; ++tries) {
        token = exchange(card, 0xff);
        if (card->io_failed || token != 0xff) {
            break;
        }
        if ((uint32_t)(card->bus.now_ms(card->bus.context) - start) >= 500U) {
            break;
        }
    }
    if (token != 0xfe || card->io_failed) {
        return false;
    }
    uint8_t tx_bytes[32];
    memset(tx_bytes, 0xff, sizeof(tx_bytes));
    for (size_t offset = 0; offset < size;) {
        size_t count = size - offset;
        if (count > sizeof(tx_bytes)) {
            count = sizeof(tx_bytes);
        }
        if (!card->bus.transfer(card->bus.context, tx_bytes, data + offset, count)) {
            card->io_failed = true;
            return false;
        }
        offset += count;
    }
    (void)exchange(card, 0xff);
    (void)exchange(card, 0xff); /* CRC disabled in SPI mode */
    return !card->io_failed;
}
/* Cold/warm startup: idle -> interface check -> ACMD41 -> addressing mode. */
bool sd_spi_init(sd_spi_t *card, const sd_spi_bus_t *bus)
{
    if (!card || !bus || !bus->transfer || !bus->select || !bus->set_fast || !bus->now_ms ||
        !bus->delay_ms) {
        return false;
    }
    memset(card, 0, sizeof(*card));
    card->bus = *bus;
    if (!card->bus.set_fast(card->bus.context, false)) {
        return false;
    }
    card->bus.select(card->bus.context, false);
    card->bus.delay_ms(card->bus.context, 10);
    for (unsigned i = 0; i < 10; ++i) {
        (void)exchange(card, 0xff);
    }
    uint32_t start = bus->now_ms(bus->context);
    bool idle = false;
    for (unsigned i = 0; i < 2000; ++i) {
        if (send_command(card, SD_CMD_GO_IDLE, 0) == 1) {
            idle = true;
            break;
        }
        if (card->io_failed || (uint32_t)(bus->now_ms(bus->context) - start) >= 2000U) {
            break;
        }
        bus->delay_ms(bus->context, 1);
    }
    if (!idle) {
        deselect_card(card);
        return false;
    }
    /* Older SDSC cards reject CMD8; SD v2 cards return the voltage/check pattern. */
    uint8_t response = send_command(card, SD_CMD_SEND_IF_COND, 0x1aa);
    bool v2 = response == 1;
    if (v2) {
        uint8_t r7[4];
        for (unsigned i = 0; i < 4; ++i) {
            r7[i] = exchange(card, 0xff);
        }
        if (r7[2] != 1 || r7[3] != 0xaa) {
            deselect_card(card);
            return false;
        }
    } else if (response != 5) {
        deselect_card(card);
        return false;
    }
    start = bus->now_ms(bus->context);
    bool ready = false;
    for (unsigned i = 0; i < 2000; ++i) {
        if (send_command(card, SD_CMD_APP_CMD, 0) > 1) {
            break;
        }
        response = send_command(card, SD_ACMD_SEND_OP_COND, v2 ? 0x40000000U : 0);
        if (response == 0) {
            ready = true;
            break;
        }
        if (response != 1 || card->io_failed ||
            (uint32_t)(bus->now_ms(bus->context) - start) >= 2000U) {
            break;
        }
        bus->delay_ms(bus->context, 1);
    }
    if (!ready) {
        deselect_card(card);
        return false;
    }
    if (send_command(card, SD_CMD_READ_OCR, 0) != 0) {
        deselect_card(card);
        return false;
    }
    uint8_t ocr[4];
    for (unsigned i = 0; i < 4; ++i) {
        ocr[i] = exchange(card, 0xff);
    }
    if (!(ocr[0] & 0x80U) || !(ocr[1] & 0x30U)) {
        deselect_card(card);
        return false;
    }
    /* OCR CCS selects sector addressing; legacy cards take byte addresses. */
    card->block_addressed = v2 && (ocr[0] & 0x40U);
    if (!card->block_addressed && send_command(card, SD_CMD_SET_BLOCKLEN, 512) != 0) {
        deselect_card(card);
        return false;
    }
    deselect_card(card);
    card->initialized = !card->io_failed && bus->set_fast(bus->context, true);
    return card->initialized;
}
static bool sector_argument(sd_spi_t *card, uint32_t sector, uint32_t *argument)
{
    if (!card || !card->initialized || card->io_failed) {
        return false;
    }
    if (!card->block_addressed && sector > UINT32_MAX / 512U) {
        return false;
    }
    *argument = card->block_addressed ? sector : sector * 512U;
    return true;
}
bool sd_spi_read(sd_spi_t *card, uint32_t sector, uint8_t *data)
{
    uint32_t argument;
    if (!data || !sector_argument(card, sector, &argument)) {
        return false;
    }
    bool succeeded = send_command(card, SD_CMD_READ_SINGLE_BLOCK, argument) == 0 &&
                     receive_data(card, data, 512);
    deselect_card(card);
    if (!succeeded || card->io_failed) {
        card->initialized = false;
        return false;
    }
    return true;
}
bool sd_spi_write(sd_spi_t *card, uint32_t sector, const uint8_t *data)
{
    uint32_t argument;
    if (!data || !sector_argument(card, sector, &argument)) {
        return false;
    }
    bool succeeded = send_command(card, SD_CMD_WRITE_SINGLE_BLOCK, argument) == 0;
    if (succeeded) {
        (void)exchange(card, 0xff);
        (void)exchange(card, 0xfe);
        uint8_t rx_bytes[32];
        for (size_t offset = 0; offset < 512 && !card->io_failed; offset += sizeof(rx_bytes)) {
            if (!card->bus.transfer(card->bus.context, data + offset, rx_bytes, sizeof(rx_bytes))) {
                card->io_failed = true;
            }
        }
        (void)exchange(card, 0xff);
        (void)exchange(card, 0xff);
        uint8_t response = exchange(card, 0xff);
        succeeded = (response & 0x1fU) == 5 && wait_ready(card, 1000);
        /* CMD13 checks programming status after busy clears. */
        if (succeeded) {
            succeeded = send_command(card, SD_CMD_SEND_STATUS, 0) == 0 && exchange(card, 0xff) == 0;
        }
    }
    deselect_card(card);
    if (!succeeded || card->io_failed) {
        card->initialized = false;
        return false;
    }
    return true;
}
bool sd_spi_sync(sd_spi_t *card)
{
    if (!card || !card->initialized) {
        return false;
    }
    card->bus.select(card->bus.context, true);
    bool succeeded = wait_ready(card, 1000);
    deselect_card(card);
    if (!succeeded || card->io_failed) {
        card->initialized = false;
        return false;
    }
    return true;
}
void sd_spi_bind(sd_spi_t *card, const sd_spi_bus_t *bus)
{
    bound_card = card;
    if (card) {
        memset(card, 0, sizeof(*card));
    }
    if (bus) {
        bound_bus = *bus;
    } else {
        memset(&bound_bus, 0, sizeof(bound_bus));
    }
}
/* FatFs diskio entry points. Binding is deliberately limited to one card. */
DSTATUS disk_status(BYTE drive)
{
    return drive || !bound_card || !bound_card->initialized ? STA_NOINIT : 0;
}
DSTATUS disk_initialize(BYTE drive)
{
    if (drive || !bound_card || !sd_spi_init(bound_card, &bound_bus)) {
        return STA_NOINIT;
    }
    return 0;
}
DRESULT disk_read(BYTE drive, BYTE *data, LBA_t sector, UINT count)
{
    if (drive || !data || !count || sector > UINT32_MAX - (count - 1U)) {
        return RES_PARERR;
    }
    if (disk_status(drive)) {
        return RES_NOTRDY;
    }
    for (UINT i = 0; i < count; ++i) {
        if (!sd_spi_read(bound_card, (uint32_t)sector + i, data + 512U * i)) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}
DRESULT disk_write(BYTE drive, const BYTE *data, LBA_t sector, UINT count)
{
    if (drive || !data || !count || sector > UINT32_MAX - (count - 1U)) {
        return RES_PARERR;
    }
    if (disk_status(drive)) {
        return RES_NOTRDY;
    }
    for (UINT i = 0; i < count; ++i) {
        if (!sd_spi_write(bound_card, (uint32_t)sector + i, data + 512U * i)) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}
DRESULT disk_ioctl(BYTE drive, BYTE command_index, void *data)
{
    if (disk_status(drive)) {
        return RES_NOTRDY;
    }
    if (command_index == CTRL_SYNC) {
        return sd_spi_sync(bound_card) ? RES_OK : RES_ERROR;
    }
    if (!data) {
        return RES_PARERR;
    }
    if (command_index == GET_SECTOR_SIZE) {
        *(WORD *)data = 512;
        return RES_OK;
    }
    if (command_index == GET_BLOCK_SIZE) {
        *(DWORD *)data = 1;
        return RES_OK;
    }
    /* Capacity queries are unnecessary for configured read/write-only FatFs.
     * Formatting/TRIM intentionally unsupported; never format flight media. */
    return RES_PARERR;
}
