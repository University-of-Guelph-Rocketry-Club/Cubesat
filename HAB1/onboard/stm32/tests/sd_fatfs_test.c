#include "../modules/sd_logger/sd_fatfs.h"
#include "diskio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Real FatFs over an isolated FAT16 RAM disk; never accesses physical media. */
#define SECTORS 32768U
static BYTE image[SECTORS * 512U];
static bool disk_fail;
static uint32_t now;
static void word(size_t offset, unsigned value)
{
    image[offset] = (BYTE)value;
    image[offset + 1] = (BYTE)(value >> 8);
}
DSTATUS disk_status(BYTE p)
{
    return p ? STA_NOINIT : 0;
}
DSTATUS disk_initialize(BYTE p)
{
    return disk_status(p);
}
DRESULT disk_read(BYTE p, BYTE *data, LBA_t sector, UINT count)
{
    if (p || sector >= SECTORS || count > SECTORS - sector) {
        return RES_PARERR;
    }
    if (disk_fail) {
        return RES_ERROR;
    }
    memcpy(data, image + sector * 512U, count * 512U);
    return RES_OK;
}
DRESULT disk_write(BYTE p, const BYTE *data, LBA_t sector, UINT count)
{
    if (p || sector >= SECTORS || count > SECTORS - sector) {
        return RES_PARERR;
    }
    if (disk_fail) {
        return RES_ERROR;
    }
    memcpy(image + sector * 512U, data, count * 512U);
    return RES_OK;
}
DRESULT disk_ioctl(BYTE p, BYTE cmd, void *data)
{
    (void)data;
    return p || disk_fail || cmd != CTRL_SYNC ? RES_ERROR : RES_OK;
}
static uint32_t clock_ms(void *p)
{
    (void)p;
    return now;
}
int main(void)
{
    image[0] = 0xeb;
    image[1] = 0x3c;
    image[2] = 0x90;
    memcpy(image + 3, "MSDOS5.0", 8);
    word(11, 512);
    image[13] = 4;
    word(14, 1);
    image[16] = 2;
    word(17, 512);
    word(19, SECTORS);
    image[21] = 0xf8;
    word(22, 32);
    word(24, 32);
    word(26, 64);
    image[38] = 0x29;
    memcpy(image + 54, "FAT16   ", 8);
    word(510, 0xaa55);
    word(512, 0xfff8);
    word(514, 0xffff);
    word(33U * 512, 0xfff8);
    word(33U * 512 + 2, 0xffff);
    sd_fatfs_t adapter;
    sd_logger_t logger;
    sd_log_storage_t storage = sd_fatfs_storage(&adapter, clock_ms, NULL);
    assert(sd_logger_init(&logger, &storage) == SD_LOG_OK);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    hab_telemetry_record_t r = {0};
    r.packet_version = 1;
    r.sequence_number = 42;
    assert(sd_logger_append(&logger, &r) == SD_LOG_OK);
    assert(sd_logger_stop(&logger) == SD_LOG_OK);
    FIL file;
    char data[1024];
    UINT got;
    assert(f_open(&file, "LOG00001.CSV", FA_READ) == FR_OK);
    assert(f_read(&file, data, sizeof(data) - 1, &got) == FR_OK);
    data[got] = '\0';
    assert(strstr(data, "1,1,42,0,"));
    assert(f_close(&file) == FR_OK);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    assert(strcmp(logger.status.filename, "LOG00002.CSV") == 0);
    assert(sd_logger_append(&logger, &r) == SD_LOG_OK);
    disk_fail = true;
    assert(sd_logger_sync(&logger) == SD_LOG_IO_ERROR);
    assert(!logger.status.has_synced_record);
    assert(sd_logger_stop(&logger) == SD_LOG_IO_ERROR);
    disk_fail = false;
    /* Fill a separate scratch file to force a real FAT allocation short write. */
    assert(sd_logger_init(&logger, &storage) == SD_LOG_OK);
    assert(sd_logger_start(&logger) == SD_LOG_OK);
    assert(f_open(&file, "FILL.BIN", FA_WRITE | FA_CREATE_NEW) == FR_OK);
    static char fill[4096];
    UINT written;
    do {
        assert(f_write(&file, fill, sizeof(fill), &written) == FR_OK);
    } while (written == sizeof(fill));
    assert(f_close(&file) == FR_OK);
    /* Data fits in FIL cache first; sync or a later sector flush sees full media. */
    sd_log_result_t result = SD_LOG_OK;
    for (unsigned i = 0; i < 100 && result == SD_LOG_OK; ++i) {
        result = sd_logger_append(&logger, &r);
    }
    assert(result == SD_LOG_FULL);
    assert(sd_logger_stop(&logger) == SD_LOG_FULL);
    puts("PASS: real FatFs FAT16 files, exclusive boot names, disk failure and full media");
    return 0;
}
