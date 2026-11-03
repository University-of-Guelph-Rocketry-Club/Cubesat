/* Host test shim only. Not a scheduler or production FreeRTOS replacement. */
#ifndef HAB_TEST_FREERTOS_H
#define HAB_TEST_FREERTOS_H
#include <stdint.h>
#include <stddef.h>
typedef int BaseType_t;
typedef unsigned UBaseType_t;
typedef uint32_t TickType_t;
#define pdPASS 1
#define pdFAIL 0
typedef struct {
    uint8_t *data;
    unsigned size, capacity, head, count;
} StaticQueue_t;
typedef StaticQueue_t *QueueHandle_t;
#endif
