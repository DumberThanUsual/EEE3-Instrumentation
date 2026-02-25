#include "stm32f446xx.h"
#include "signal.h"

#include "dac.h"
#include "tim.h"

#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"

#include "math.h"
#include "arm_math.h"

typedef struct {
    float c, s;   // cos(ω0), sin(ω0)
    float x, y;   // state (cos, sin) scaled to DAC counts

    uint16_t buffer[2 * HALF_BUF_LEN];
} osc_f32_t;

osc_f32_t state;

void signal_set_freq(osc_f32_t *state, float f0) 
{
    float w = 2.0f * (float)PI * (f0 / (float)DAC_FS);
    state->c = cosf(w);
    state->s = sinf(w);
}

void signal_set_phase_amp_counts(osc_f32_t *state, float phi, float A_counts) 
{
    state->x = A_counts * cosf(phi);
    state->y = A_counts * sinf(phi);
}

float signal_step(osc_f32_t *state) 
{
    // one exact rotation of the state vector corresponds to one period of the output sine wave, so we can use the rotation matrix to update the state
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

void signal_stop()
{
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, DAC_MID);
}

void signal_start(float frequency, uint16_t amplitude)
{
    signal_stop();

    signal_set_freq(&state, frequency);
    signal_set_phase_amp_counts(&state, 0.0f, amplitude);

    for (uint32_t i = 0; i < HALF_BUF_LEN; i ++) { 
        float y = signal_step(&state);
        state.buffer[i] = to_dac12_clamped(y);
    }

    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)state.buffer, 2 * HALF_BUF_LEN, DAC_ALIGN_12B_R);
}

void signal_task(void *pvParameters) 
{
    uint16_t *buffer;

    while (1) {
        uint32_t half = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (half)
            buffer = state.buffer;
        else
            buffer = state.buffer + HALF_BUF_LEN;

        for (uint32_t i = 0; i < HALF_BUF_LEN; i ++) { 
            float y = signal_step(&state);
            buffer[i] = to_dac12_clamped(y);
        }

    }
}

void signal_init()
{
    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_OC_Start(&htim2, TIM_CHANNEL_1);

    xTaskCreate(
        signal_task,
        "signal_task",
        1024,
        NULL,
        1,
        &signal_task_handle
    );

    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, DAC_MID);
}

BaseType_t signal_DMA_Handler(uint32_t half)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xHigherPriorityTaskWoken = xTaskNotifyFromISR(signal_task_handle, half, eSetValueWithOverwrite, NULL);
    return xHigherPriorityTaskWoken;
}