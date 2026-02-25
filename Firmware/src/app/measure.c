#include "measure.h"

void dac_buffer_task(void *pvParameters);

void measure_stop()
{
    HAL_ADCEx_MultiModeStop_DMA(&hadc1);
    HAL_ADC_Stop_DMA(&hadc2);

    __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_OVR | ADC_FLAG_EOC);
    __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_OVR | ADC_FLAG_EOC);
    
    return;
}

void measure_start(uint32_t* buffer, uint32_t length) 
{
    HAL_ADC_Start(&hadc2);
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, buffer, length);
    return;
}

void measure_task(void *pvParameters) 
{
    measurement_request_t req;
    //measure_setup(&req);

    while (1) {
        xQueueReceive(measurement_queue, &req, portMAX_DELAY);
        signal_start(req.frequency, req.amplitude);
        measure_start(req.result_buffer, req.result_length);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        measure_stop();
        signal_stop();
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
    return;
}
