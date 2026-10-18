#pragma once
#include <stddef.h>
typedef struct fake_queue *QueueHandle_t;
QueueHandle_t xQueueCreate(unsigned count,size_t size);
int xQueueSend(QueueHandle_t q,const void *item,unsigned wait);
int xQueueReceive(QueueHandle_t q,void *item,unsigned wait);
int xQueueOverwrite(QueueHandle_t q,const void *item);
void vQueueDelete(QueueHandle_t q);
