#ifndef SIGNAL_H
#define SIGNAL_H

#include "stm32f446xx.h"
#include "FreeRTOS.h"
#include "task.h"

#define DAC_FS 1000000
#define HALF_BUF_LEN 1024
#define DAC_MID 2048

TaskHandle_t signal_task_handle;

BaseType_t signal_DMA_Handler(uint32_t half);
void signal_start(float frequency, uint16_t amplitude);
void signal_stop();

#endif