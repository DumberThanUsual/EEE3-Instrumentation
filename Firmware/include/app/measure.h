#ifndef MEASURE_H
#define MEASURE_H

#include "stm32f446xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "math.h"
#include "signal.h"
#include "adc.h"
#include "dac.h"

typedef struct {
    uint32_t *result_buffer;
    uint32_t result_length;
    SemaphoreHandle_t complete;
} measurement_request_t;

QueueHandle_t measurement_queue;

TaskHandle_t measure_task_handle;

void measure_init();

#endif