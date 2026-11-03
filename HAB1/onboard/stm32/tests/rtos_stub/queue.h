#include "FreeRTOS.h"
QueueHandle_t xQueueCreateStatic(UBaseType_t, UBaseType_t, uint8_t *, StaticQueue_t *);
BaseType_t xQueueSend(QueueHandle_t, const void *, TickType_t);
BaseType_t xQueueReceive(QueueHandle_t, void *, TickType_t);
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t);
