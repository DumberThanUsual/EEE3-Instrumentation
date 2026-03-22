#include "measure.h"
#include "process.h"
#include "tim.h"
#include "stm32f446xx.h"
#include <string.h>

QueueHandle_t measurement_queue;
TaskHandle_t measure_task_handle;
measurement_voltage_gain_t measurement_voltage_range;

uint32_t sample_buffer[SAMPLE_BUFFER_SIZE];
SemaphoreHandle_t buffer_in_use;
SemaphoreHandle_t complete;

setup_t setup;
setup_t buffered_autorange_setup;
setup_t buffered_new_setup;

result_t result;

void dac_buffer_task(void *pvParameters);

void measure_stop()
{
    HAL_TIM_Base_Stop(&htim3);
    HAL_ADCEx_MultiModeStop_DMA(&hadc1);
    HAL_ADC_Stop(&hadc2);
    
    __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_OVR | ADC_FLAG_EOC);
    __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_OVR | ADC_FLAG_EOC);
    
    return;
}

HAL_StatusTypeDef measure_start() 
{   
    measure_stop();

    HAL_StatusTypeDef s2 = HAL_ADC_Start(&hadc2);
    HAL_StatusTypeDef s1 = HAL_ADCEx_MultiModeStart_DMA(&hadc1, sample_buffer, SAMPLE_BUFFER_SIZE);

    HAL_TIM_Base_Start_IT(&htim3);
    //HAL_TIM_PWM_Start_IT(&htim3, TIM_CHANNEL_1);
    return (s1 == HAL_OK && s2 == HAL_OK) ? HAL_OK : HAL_ERROR;
}

/**
* @brief Setup signal source switches
* @param voltage_peak output voltage amplitude in mV
* @retval ADC amplitude in counts
*/
uint16_t measure_setup_source(uint16_t voltage_peak)
{
    const float voltage_to_counts = (4096.0/3300.0) * 0.9;
    
    if (voltage_peak > 1500) {
        // x4
        HAL_GPIO_WritePin(SOURCE_nMUL4_GPIO_Port, SOURCE_nMUL4_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SOURCE_DIV10_GPIO_Port, SOURCE_DIV10_Pin, GPIO_PIN_RESET);
        return (uint16_t)(((float)voltage_peak * voltage_to_counts) / 4);
    } else if (voltage_peak > 600) {
        // x1
        HAL_GPIO_WritePin(SOURCE_nMUL4_GPIO_Port, SOURCE_nMUL4_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(SOURCE_DIV10_GPIO_Port, SOURCE_DIV10_Pin, GPIO_PIN_RESET);
        return (uint16_t)(((float)voltage_peak * voltage_to_counts));
    } else if (voltage_peak > 150) {
        // x0.4
        HAL_GPIO_WritePin(SOURCE_nMUL4_GPIO_Port, SOURCE_nMUL4_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SOURCE_DIV10_GPIO_Port, SOURCE_DIV10_Pin, GPIO_PIN_SET);
        return (uint16_t)((float)voltage_peak * voltage_to_counts * 2.5);
    } else {
        // x0.1
        HAL_GPIO_WritePin(SOURCE_nMUL4_GPIO_Port, SOURCE_nMUL4_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(SOURCE_DIV10_GPIO_Port, SOURCE_DIV10_Pin, GPIO_PIN_SET);
        return (uint16_t)((float)voltage_peak * voltage_to_counts * 10);
    }
}

/**
* @brief Setup voltage measurement switches
* @param 
*/
void measure_setup_voltage(measurement_voltage_gain_t gain)
{
    switch (gain){
    case MV_x0_25:
        // Vpk < 6000mV
        HAL_GPIO_WritePin(VAMP_DIV4_GPIO_Port, VAMP_DIV4_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL1_GPIO_Port, VAMP_nMUL1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(VAMP_nMUL3_GPIO_Port, VAMP_nMUL3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL100_GPIO_Port, VAMP_nMUL100_Pin, GPIO_PIN_SET);
        break;
    case MV_x1:
        //Vpk < 1500mV
        HAL_GPIO_WritePin(VAMP_DIV4_GPIO_Port, VAMP_DIV4_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(VAMP_nMUL1_GPIO_Port, VAMP_nMUL1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(VAMP_nMUL3_GPIO_Port, VAMP_nMUL3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL100_GPIO_Port, VAMP_nMUL100_Pin, GPIO_PIN_SET);
        break;
    case MV_x2_5:
        // Vpk < 600mV
        HAL_GPIO_WritePin(VAMP_DIV4_GPIO_Port, VAMP_DIV4_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL1_GPIO_Port, VAMP_nMUL1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL3_GPIO_Port, VAMP_nMUL3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL100_GPIO_Port, VAMP_nMUL100_Pin, GPIO_PIN_SET);
        break;
    case MV_x10:
        // Vpk < 150mV
        HAL_GPIO_WritePin(VAMP_DIV4_GPIO_Port, VAMP_DIV4_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(VAMP_nMUL1_GPIO_Port, VAMP_nMUL1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL3_GPIO_Port, VAMP_nMUL3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL100_GPIO_Port, VAMP_nMUL100_Pin, GPIO_PIN_SET);
        break;
    case MV_x25:
        // Vpk < 60mV
        HAL_GPIO_WritePin(VAMP_DIV4_GPIO_Port, VAMP_DIV4_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL1_GPIO_Port, VAMP_nMUL1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL3_GPIO_Port, VAMP_nMUL3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL100_GPIO_Port, VAMP_nMUL100_Pin, GPIO_PIN_RESET);
        break;
    case MV_x100:
        // Vpk < 15mV
        HAL_GPIO_WritePin(VAMP_DIV4_GPIO_Port, VAMP_DIV4_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(VAMP_nMUL1_GPIO_Port, VAMP_nMUL1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL3_GPIO_Port, VAMP_nMUL3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(VAMP_nMUL100_GPIO_Port, VAMP_nMUL100_Pin, GPIO_PIN_RESET);
        break;
    }
}


/**
 * @brief Setup voltage measurement switches
 * @param 
 */
measurement_voltage_gain_t measure_set_meas_voltage(uint16_t voltage_peak)
{

    if (voltage_peak > 1500) {
        // Vpk < 6000mV
        // x0.25
        measure_setup_voltage(MV_x0_25);
        return MV_x0_25;
    } else if (voltage_peak > 600) {
        // Vpk < 1500mV
        // x1
        measure_setup_voltage(MV_x1);
        return MV_x1;
    } else if (voltage_peak > 150) {
        // Vpk < 600mV
        // x2.5
        measure_setup_voltage(MV_x2_5);
        return MV_x0_25;
    } else if (voltage_peak > 60) {
        // Vpk < 150mV
        // x10
        measure_setup_voltage(MV_x10);
        return MV_x10;
    } else if (voltage_peak > 15) {
        // Vpk < 60mV
        // x25
        measure_setup_voltage(MV_x25);
        return MV_x25;
    } else {
        // Vpk < 15mV
        // x100
        measure_setup_voltage(MV_x100);
        return MV_x100;
    }
}

inline static void iamp_mul1() 
{
    HAL_GPIO_WritePin(IAMP_DIV4_GPIO_Port, IAMP_DIV4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IAMP_nMUL1_GPIO_Port, IAMP_nMUL1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IAMP_nMUL3_GPIO_Port, IAMP_nMUL3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_nMUL100_GPIO_Port, IAMP_nMUL100_Pin, GPIO_PIN_SET);
}

inline static void iamp_mul3() 
{
    HAL_GPIO_WritePin(IAMP_DIV4_GPIO_Port, IAMP_DIV4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IAMP_nMUL1_GPIO_Port, IAMP_nMUL1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_nMUL3_GPIO_Port, IAMP_nMUL3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IAMP_nMUL100_GPIO_Port, IAMP_nMUL100_Pin, GPIO_PIN_SET);
}

inline static void iamp_mul10() 
{
    HAL_GPIO_WritePin(IAMP_DIV4_GPIO_Port, IAMP_DIV4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IAMP_nMUL1_GPIO_Port, IAMP_nMUL1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_nMUL3_GPIO_Port, IAMP_nMUL3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_nMUL100_GPIO_Port, IAMP_nMUL100_Pin, GPIO_PIN_SET);
}

inline static void iamp_mul30() 
{
    HAL_GPIO_WritePin(IAMP_DIV4_GPIO_Port, IAMP_DIV4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IAMP_nMUL1_GPIO_Port, IAMP_nMUL1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_nMUL3_GPIO_Port, IAMP_nMUL3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IAMP_nMUL100_GPIO_Port, IAMP_nMUL100_Pin, GPIO_PIN_SET);
}

inline static void iamp_mul100()
{
    HAL_GPIO_WritePin(IAMP_DIV4_GPIO_Port, IAMP_DIV4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IAMP_nMUL1_GPIO_Port, IAMP_nMUL1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_nMUL3_GPIO_Port, IAMP_nMUL3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_nMUL100_GPIO_Port, IAMP_nMUL100_Pin, GPIO_PIN_RESET);
}

void measure_setup_current(measurement_current_gain_t gain)
{
    switch(gain) {
    case MI_10Z:
        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul1();
        break;
        
    case MI_30Z:
        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul3();
        break;
        
    case MI_100Z:
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul1();
        break;
        
    case MI_300Z:
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));
        
        iamp_mul3();
        break;
        
    case MI_1KZ:
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul1();
        break;
        
    case MI_3KZ:
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul3();
        break;
        
    case MI_10KZ:
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul1();
        break;
        
    case MI_30KZ:
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul3();
        break;
        
    case MI_100KZ:
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul1();
        break;
        
    case MI_300KZ:
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_SET);

        vTaskDelay(pdMS_TO_TICKS(5));

        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul3();
        break;
        
    case MI_1MZ:
        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul1();
        break;
        
    case MI_3MZ:
        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul3();
        break;
        
    case MI_10MZ:
        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul10();
        break;
        
    case MI_30MZ:
        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul30();
        break;
        
    case MI_100MZ:
        HAL_GPIO_WritePin(RANGE_10_GPIO_Port, RANGE_10_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100_GPIO_Port, RANGE_100_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_1K_GPIO_Port, RANGE_1K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RANGE_100K_GPIO_Port, RANGE_100K_Pin, GPIO_PIN_RESET);

        vTaskDelay(pdMS_TO_TICKS(5));

        iamp_mul100();
        break;        
    }
}

#define APB2_CLOCK 70000000

void measure_setup_ADC(float frequency) 
{
    //adjust ADC sampling rate to 20 times the signal frequency up to 2MHz
    float adc_sampling_rate = fminf(frequency * 20, 2000000);
    setup.sample_rate = adc_sampling_rate;
    uint32_t timer_period = (APB2_CLOCK / adc_sampling_rate) - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim3, timer_period);
}

void measure_task(void *pvParameters) 
{    
    uint8_t rerange;
    uint8_t range_attempts;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (buffered_new_setup.auto_voltage_gain) {
            buffered_new_setup.voltage_gain = setup.voltage_gain;
        }

        if (buffered_new_setup.auto_current_gain) {
            buffered_new_setup.current_gain = setup.current_gain;
        }
        memcpy(&setup, &buffered_new_setup, sizeof(setup_t));
        uint16_t dac_amp = measure_setup_source(setup.source_amplitude);
        rerange = 0;
        range_attempts = 0;
        do{
            measure_setup_voltage(setup.voltage_gain);
            measure_setup_current(setup.current_gain);
            //measure_setup_ADC(setup.source_frequency);
            signal_start(setup.source_frequency, dac_amp);
            measure_start();
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            measure_stop();
            signal_stop();
            rerange = process();
            range_attempts++;
        } while (rerange && range_attempts < 10);

        xSemaphoreGive(complete);
    }
    
    vTaskDelete(NULL);
}

BaseType_t measure_init()
{
    complete = xSemaphoreCreateBinary();
    buffer_in_use = xSemaphoreCreateBinary();

    return xTaskCreate(
        measure_task,
        "measure_task",
        2048,
        NULL,
        1,
        &measure_task_handle
    );
}
