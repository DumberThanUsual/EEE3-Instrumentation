#ifndef BIAS_H
#define BIAS_H

#include "stm32f446xx.h"
#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"

typedef enum {
    BIAS_VOLTAGE,
    BIAS_CURRENT,
} bias_type_t;

typedef struct {
    uint32_t value;
    bias_type_t type;
} bias_t;

static void bias_task(void *pvParameters);

void set_bias(bias_t *bias);

#endif