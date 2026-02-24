#ifndef MEASURE_H
#define MEASURE_H

#include "stm32f446xx.h"
#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"
#include "semphr.h"

typedef enum {
    M_RANGE_
} measurement_range_t;

typedef enum {
    M_OFFSET_VOLTAGE,
    M_OFFSET_CURRENT,
} measurement_offset_type_t;

typedef struct {
    uint32_t frequency;
    uint32_t amplitude;
    measurement_range_t range;
    uint16_t offset;
    measurement_offset_type_t offset_type;

    uint32_t *result_buffer;
    uint32_t result_length;
    SemaphoreHandle_t complete;
} measurement_request_t;

QueueHandle_t measurement_queue;

TaskHandle_t measure_task_handle;
TaskHandle_t dac_buffer_task_handle;

void measure_init();

BaseType_t osc_DMA_Handler(uint32_t half);
void osc_start(float frequency, uint16_t amplitude);
void osc_stop();

#endif