/* C-only NUCLEO-F411RE bench application. Synthetic records, not sensor data. */
#include "stm32f4xx_hal.h"
#include "sd_spi_hal.h"
#include "sd_fatfs.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static SPI_HandleTypeDef spi;
static UART_HandleTypeDef uart;
static sd_spi_t card;
static sd_fatfs_t filesystem;
static sd_logger_t logger;
static FIL reader;
static bool continuous;
static uint32_t clock_ms(void *p)
{
    (void)p;
    return HAL_GetTick();
}
static void print(const char *text)
{
    (void)HAL_UART_Transmit(&uart, (uint8_t *)text, (uint16_t)strlen(text), 1000);
}
void SysTick_Handler(void)
{
    HAL_IncTick();
}
static void failed(const char *stage, sd_log_result_t code)
{
    char line[160];
    (void)snprintf(line, sizeof(line), "FAIL %s result=%u fatfs=%u cmd=%u response=0x%02x\r\n",
                   stage, (unsigned)code, (unsigned)filesystem.last_result, card.last_command,
                   card.last_response);
    print(line);
    (void)sd_logger_stop(&logger);
    for (;;) {
        HAL_Delay(1000);
    }
}
static void board_setup(void)
{
    HAL_Init(); /* HSI 16 MHz; no external crystal or PLL dependency. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Alternate = GPIO_AF5_SPI1;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio);
    uart.Instance = USART2;
    uart.Init.BaudRate = 115200;
    uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1;
    uart.Init.Parity = UART_PARITY_NONE;
    uart.Init.Mode = UART_MODE_TX_RX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart.Init.OverSampling = UART_OVERSAMPLING_16;
    (void)HAL_UART_Init(&uart);
    spi.Instance = SPI1;
    spi.Init.Mode = SPI_MODE_MASTER;
    spi.Init.Direction = SPI_DIRECTION_2LINES;
    spi.Init.DataSize = SPI_DATASIZE_8BIT;
    spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi.Init.NSS = SPI_NSS_SOFT;
    spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64; /* 250 kHz */
    spi.Init.TIMode = SPI_TIMODE_DISABLE;
    spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    (void)HAL_SPI_Init(&spi);
}
static void verify_file(const char *name, unsigned expected_rows)
{
    if (f_open(&reader, name, FA_READ) != FR_OK) {
        failed("reopen", SD_LOG_IO_ERROR);
    }
    char buffer[256];
    UINT count;
    unsigned lines = 0;
    do {
        if (f_read(&reader, buffer, sizeof(buffer), &count) != FR_OK) {
            failed("read", SD_LOG_IO_ERROR);
        }
        for (UINT i = 0; i < count; ++i) {
            if (buffer[i] == '\n') {
                ++lines;
            }
        }
    } while (count);
    (void)f_close(&reader);
    if (lines != expected_rows + 1U) {
        failed("row-count", SD_LOG_INVALID);
    }
    print("PASS: closed/reopened file and verified complete row count\r\n");
}
static void audit_files(void)
{
    /* Inspect earlier sessions without changing them. Hashes make accidental
     * overwrites visible; they are a bench check, not an on-disk integrity code. */
    DIR directory;
    FILINFO info;
    if (f_opendir(&directory, "0:/") != FR_OK) {
        failed("directory", SD_LOG_IO_ERROR);
    }
    for (;;) {
        if (f_readdir(&directory, &info) != FR_OK) {
            failed("list", SD_LOG_IO_ERROR);
        }
        if (!info.fname[0]) {
            break;
        }
        if (strncmp(info.fname, "LOG", 3) || !strstr(info.fname, ".CSV")) {
            continue;
        }
        if (f_open(&reader, info.fname, FA_READ) != FR_OK) {
            failed("audit-open", SD_LOG_IO_ERROR);
        }
        uint32_t hash = UINT32_C(2166136261), bytes = 0, rows = 0, sequence = 0, time = 0;
        char row[512];
        size_t used = 0;
        bool overflow = false;
        uint8_t data[128];
        UINT count;
        do {
            if (f_read(&reader, data, sizeof(data), &count) != FR_OK) {
                failed("audit-read", SD_LOG_IO_ERROR);
            }
            for (UINT i = 0; i < count; ++i) {
                hash = (hash ^ data[i]) * UINT32_C(16777619);
                ++bytes;
                if (data[i] == '\n') {
                    row[used] = '\0';
                    uint32_t seq, ms;
                    if (!overflow && sscanf(row, "1,1,%" SCNu32 ",%" SCNu32, &seq, &ms) == 2) {
                        ++rows;
                        sequence = seq;
                        time = ms;
                    }
                    used = 0;
                    overflow = false;
                } else if (used < sizeof(row) - 1U) {
                    row[used++] = (char)data[i];
                } else {
                    overflow = true;
                }
            }
        } while (count);
        (void)f_close(&reader);
        char line[180];
        (void)snprintf(line, sizeof(line),
                       "AUDIT %s bytes=%" PRIu32 " hash=%08" PRIx32 " rows=%" PRIu32
                       " last_seq=%" PRIu32 " last_ms=%" PRIu32 " partial=%u\r\n",
                       info.fname, bytes, hash, rows, sequence, time,
                       (unsigned)(used != 0 || overflow));
        print(line);
    }
    (void)f_closedir(&directory);
}
int main(void)
{
    board_setup();
    HAL_Delay(1500);
    print("URC-43 C/FatFs logger bench; SYNTHETIC records\r\n");
    sd_spi_hal_t port = {&spi, GPIOB, GPIO_PIN_6, SPI_BAUDRATEPRESCALER_64,
                         SPI_BAUDRATEPRESCALER_16};
    sd_spi_bus_t bus = sd_spi_hal_bus(&port);
    sd_spi_bind(&card, &bus);
/* P starts another session in this same boot for the manual power-cut exercise.
 * Normal flight firmware creates its one session at task startup instead. */
new_session:;
    sd_log_storage_t storage = sd_fatfs_storage(&filesystem, clock_ms, NULL);
    sd_log_result_t result = sd_logger_init(&logger, &storage);
    if (result == SD_LOG_OK) {
        result = sd_logger_start(&logger);
    }
    if (result != SD_LOG_OK) {
        failed("start", result);
    }
    char text[160];
    (void)snprintf(text, sizeof(text), "OPEN %s addressing=%s\r\n", logger.status.filename,
                   card.block_addressed ? "block" : "byte");
    print(text);
    audit_files();
    uint32_t next_sample = HAL_GetTick();
    unsigned produced = 0;
    uint32_t last_synced = 0;
    while (continuous || produced < 12U) {
        uint32_t now = HAL_GetTick();
        if ((int32_t)(now - next_sample) >= 0) {
            hab_telemetry_record_t record = {0};
            record.packet_version = HAB_TELEMETRY_PACKET_VERSION;
            record.sequence_number = produced;
            record.mission_time_ms = now;
            record.temperature_c = 20.0f + (float)produced * 0.25f;
            record.pressure_pa = 101325.0f - (float)produced;
            record.valid_fields = HAB_VALID_TEMPERATURE_C | HAB_VALID_PRESSURE_PA;
            result = sd_logger_append(&logger, &record);
            if (result != SD_LOG_OK) {
                failed("append", result);
            }
            if (continuous) {
                (void)snprintf(text, sizeof(text), "SAMPLE seq=%u ms=%" PRIu32 "\r\n", produced,
                               now);
                print(text);
            }
            ++produced;
            next_sample += 1000;
        }
        result = sd_logger_poll(&logger);
        if (result != SD_LOG_OK) {
            failed("sync", result);
        }
        if (logger.status.synced_records != last_synced) {
            last_synced = logger.status.synced_records;
            (void)snprintf(text, sizeof(text),
                           "SYNC seq=%" PRIu32 " sample_ms=%" PRIu32 " done_ms=%" PRIu32 "\r\n",
                           logger.status.last_synced_sequence,
                           logger.status.last_synced_mission_time_ms, logger.status.last_sync_ms);
            print(text);
        }
        HAL_Delay(10);
    }
    if (sd_logger_stop(&logger) != SD_LOG_OK) {
        failed("close", SD_LOG_IO_ERROR);
    }
    verify_file(logger.status.filename, produced);
    /* Export the exact persisted CSV via serial for independent host checking. */
    print("CSV_BEGIN\r\n");
    if (f_open(&reader, logger.status.filename, FA_READ) != FR_OK) {
        failed("export", SD_LOG_IO_ERROR);
    }
    char data[129];
    UINT count;
    do {
        if (f_read(&reader, data, 128, &count) != FR_OK) {
            failed("export-read", SD_LOG_IO_ERROR);
        }
        data[count] = '\0';
        print(data);
    } while (count);
    (void)f_close(&reader);
    print("CSV_END\r\n");
    print("READY: send R to create another session, or P for continuous power-cut test\r\n");
    for (;;) {
        uint8_t input;
        if (HAL_UART_Receive(&uart, &input, 1, 100) == HAL_OK) {
            if (input == 'R' || input == 'r') {
                NVIC_SystemReset();
            }
            if (input == 'P' || input == 'p') {
                continuous = true;
                goto new_session;
            }
        }
    }
}
