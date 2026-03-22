#ifndef SIGNAL_H
#define SIGNAL_H

#include "stm32f446xx.h"
#include "FreeRTOS.h"
#include "task.h"

#define DAC_FS 1000000
#define HALF_BUF_LEN 1024
#define DAC_MID 2048

#define LUT_BITS       12u          // 2^10 = 1024 samples per full cycle (quarter LUT = 256)
#define LUT_SIZE       (1u << LUT_BITS)
#define LUT_QTR        (LUT_SIZE/4u)
#define FRAC_BITS      10u           // 8-bit linear interpolation within LUT cell

TaskHandle_t signal_task_handle;

BaseType_t signal_DMA_Handler(uint32_t half);
void signal_start(uint32_t frequency, uint16_t amplitude);
void signal_stop();
BaseType_t signal_init();

#endif