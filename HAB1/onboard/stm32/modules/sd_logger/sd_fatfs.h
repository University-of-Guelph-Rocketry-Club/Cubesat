#ifndef HAB_SD_FATFS_H
#define HAB_SD_FATFS_H
#include "sd_logger.h"
#include "ff.h"
typedef struct {
    FATFS filesystem;
    FIL file;
    FRESULT last_result;
    void *clock_context;
    uint32_t (*now_ms)(void *);
} sd_fatfs_t;
/* Initialize before passing callbacks to sd_logger_init. Single volume/owner. */
sd_log_storage_t sd_fatfs_storage(sd_fatfs_t *adapter, uint32_t (*now_ms)(void *),
                                  void *clock_context);
#endif
