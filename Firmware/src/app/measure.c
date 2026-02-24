#include "stm32f446xx.h"
#include "measure.h"

#include "adc.h"
#include "dac.h"

#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "math.h"

void dac_buffer_task(void *pvParameters);

void measure_stop()
{
    volatile HAL_StatusTypeDef status;
    status = HAL_ADCEx_MultiModeStop_DMA(&hadc1);
    status = HAL_ADC_Stop_DMA(&hadc2);

    __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_OVR | ADC_FLAG_EOC);
    __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_OVR | ADC_FLAG_EOC);
    
    return;
}

void measure_start(uint32_t* buffer, uint32_t length) 
{
    volatile HAL_StatusTypeDef status;
    status = HAL_ADC_Start(&hadc2);
    status = HAL_ADCEx_MultiModeStart_DMA(&hadc1, buffer, length);
    return;
}

void measure_task(void *pvParameters) 
{
    measurement_request_t req;
    //measure_setup(&req);

    while (1) {
        xQueueReceive(measurement_queue, &req, portMAX_DELAY);
        osc_start(req.frequency, req.amplitude);
        measure_start(req.result_buffer, req.result_length);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        measure_stop();
        osc_stop();
        xSemaphoreGive(req.complete);
    }

    vTaskDelete(NULL);
}

void measure_setup(measurement_request_t *req) 
{
}

void measure_init()
{
    measurement_queue = xQueueCreate(2, sizeof(measurement_request_t));

    xTaskCreate(
        measure_task,
        "measure_task",
        1024,
        NULL,
        1,
        &measure_task_handle
    );

    xTaskCreate(
        dac_buffer_task,
        "dac_buffer_task",
        1024,
        NULL,
        1,
        &dac_buffer_task_handle
    );
    return;
}

#define DAC_FS 1000000
#define HALF_BUF_LEN 1024
#define DAC_MID 2048


typedef struct {
    float c, s;   // cos(ω0), sin(ω0)
    float x, y;   // state (cos, sin) scaled to DAC counts

    uint16_t buffer[2 * HALF_BUF_LEN];
} osc_f32_t;

void osc_set_freq(osc_f32_t *state, float f0) 
{
    float w = 2.0f * (float)M_PI * (f0 / (float)DAC_FS);
    state->c = cosf(w);
    state->s = sinf(w);
}

void osc_set_phase_amp_counts(osc_f32_t *state, float phi, float A_counts) 
{
    state->x = A_counts * cosf(phi);
    state->y = A_counts * sinf(phi);
}

float osc_step(osc_f32_t *state) 
{
    // one exact rotation
    float nx = state->c * state->x - state->s * state->y;
    float ny = state->s * state->x + state->c * state->y;
    state->x = nx; state->y = ny;
    return state->y; // sine sample in "counts"
}

uint16_t to_dac12_clamped(float val_counts) 
{
    int32_t v = (int32_t)lroundf((float)DAC_MID + val_counts);
    if (v < 0) {
        v = 0;
    }
    if (v > 4095) {
        v = 4095;
    }
    return (uint16_t)v;
}

osc_f32_t state;

void osc_stop()
{
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
}

void osc_start(float frequency, uint16_t amplitude)
{
    osc_stop();

    osc_set_freq(&state, frequency);
    osc_set_phase_amp_counts(&state, 0.0f, amplitude);

    for (uint32_t i = 0; i < HALF_BUF_LEN; i ++) { 
        float y = osc_step(&state);
        state.buffer[i] = to_dac12_clamped(y);
    }

    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)state.buffer, 2 * HALF_BUF_LEN, DAC_ALIGN_12B_R);
}

void dac_buffer_task(void *pvParameters) 
{
    uint16_t *buffer;

    while (1) {
        uint32_t half = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (half)
            buffer = state.buffer;
        else
            buffer = state.buffer + HALF_BUF_LEN;

        for (uint32_t i = 0; i < HALF_BUF_LEN; i ++) { 
            float y = osc_step(&state);
            buffer[i] = to_dac12_clamped(y);
        }

    }
}

BaseType_t osc_DMA_Handler(uint32_t half)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xHigherPriorityTaskWoken = xTaskNotifyFromISR(dac_buffer_task_handle, half, eSetValueWithOverwrite, NULL);

    return xHigherPriorityTaskWoken;
}