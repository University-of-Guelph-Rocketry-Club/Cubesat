/* Translate filesystem operations/results into the portable logger contract.
 * This adapter owns volume 0 and retains the original FatFs error for diagnosis.
 */
#include "sd_fatfs.h"
#include <limits.h>
#include <string.h>
static sd_log_result_t map_result(sd_fatfs_t *adapter, FRESULT operation_result)
{
    adapter->last_result = operation_result;
    if (operation_result == FR_OK) {
        return SD_LOG_OK;
    }
    if (operation_result == FR_EXIST) {
        return SD_LOG_EXISTS;
    }
    return SD_LOG_IO_ERROR;
}
static sd_log_result_t mount_card(void *context)
{
    sd_fatfs_t *adapter = context;
    return map_result(adapter, f_mount(&adapter->filesystem, "0:", 1));
}
static sd_log_result_t create_file(void *context, const char *name)
{
    /* FA_CREATE_NEW fails on a collision; never truncate an earlier session. */
    sd_fatfs_t *adapter = context;
    return map_result(adapter, f_open(&adapter->file, name, FA_WRITE | FA_CREATE_NEW));
}
static sd_log_result_t write_file(void *context, const char *data, size_t size, size_t *written)
{
    sd_fatfs_t *adapter = context;
    if (size > UINT_MAX) {
        return SD_LOG_INVALID;
    }
    UINT bytes = 0;
    sd_log_result_t operation_result =
        map_result(adapter, f_write(&adapter->file, data, (UINT)size, &bytes));
    *written = bytes;
    /* FatFs returns FR_OK with a short count when the volume is full. */
    return operation_result == SD_LOG_OK && bytes != size ? SD_LOG_FULL : operation_result;
}
static sd_log_result_t sync_file(void *context)
{
    /* FatFs flushes file data/metadata and invokes the disk CTRL_SYNC path. */
    sd_fatfs_t *adapter = context;
    return map_result(adapter, f_sync(&adapter->file));
}
static sd_log_result_t close_file(void *context)
{
    sd_fatfs_t *adapter = context;
    sd_log_result_t operation_result = map_result(adapter, f_close(&adapter->file));
    /* Failed f_close may leave a valid object. Unmount to invalidate it before
     * any explicit recovery. This adapter owns the entire volume. */
    if (operation_result != SD_LOG_OK) {
        (void)f_mount(NULL, "0:", 0);
    }
    return operation_result;
}
static uint32_t clock_ms(void *context)
{
    sd_fatfs_t *adapter = context;
    return adapter->now_ms(adapter->clock_context);
}
sd_log_storage_t sd_fatfs_storage(sd_fatfs_t *adapter, uint32_t (*now_ms)(void *),
                                  void *clock_context)
{
    memset(adapter, 0, sizeof(*adapter));
    adapter->now_ms = now_ms;
    adapter->clock_context = clock_context;
    sd_log_storage_t storage = {adapter,   mount_card, create_file, write_file,
                                sync_file, close_file, clock_ms};
    return storage;
}
