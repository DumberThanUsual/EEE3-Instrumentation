#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
//#include "usb_otg.h"
#include "gpio.h"
#include "stm32f4xx_it.h"
#include "usart.h"
#include <stdio.h>

#include "arm_math.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "measure.h"
#include "goertzel.h"

void SystemClock_Config(void);

uint32_t sample_buffer[4096];

#define SAMPLE_RATE_HZ  2300000
#define TARGET_TONE_HZ  10000.0f

// Wrap phase to [-pi, pi]
static inline float wrap_pi(float x)
{
    while (x > (float)M_PI)  x -= 2.0f * (float)M_PI;
    while (x < -(float)M_PI) x += 2.0f * (float)M_PI;
    return x;
}

void measure_test(void *pvParameters) 
{
    uint32_t f = 1000;
    SemaphoreHandle_t measurement_complete = xSemaphoreCreateBinary();
    while (1) {
        measurement_request_t req = {
            .frequency = f,
            .amplitude = 1500,
            .range = M_RANGE_,
            .offset = 0,
            .offset_type = M_OFFSET_VOLTAGE,
            .result_buffer = sample_buffer,
            .result_length = 4096,
            .complete = measurement_complete
        };
        xQueueSendToBack(measurement_queue, &req, portMAX_DELAY);
        xSemaphoreTake(measurement_complete, portMAX_DELAY);

        volatile float magnitude1;
        volatile float magnitude2;
        volatile float phase1;
        volatile float phase2;

        //Goertzel Algorithm
        volatile float w = 2 * PI * (float)req.frequency * (15.0f / 35000000.0f);
        float c_r = arm_cos_f32(w);
        float c_i = arm_sin_f32(w);
        float s_prev_1 = 0;
        float s_prev_2 = 0;
        float s;

        float X_r;
        float X_i;

        for (uint16_t i = 0; i < req.result_length; i ++) {
            uint16_t sample = (uint16_t)(req.result_buffer[i] >> 16);
            s = (float)sample + (2 * c_r * s_prev_1) - s_prev_2;
            s_prev_2 = s_prev_1;
            s_prev_1 = s; 
        }

        X_r = (s_prev_1 * c_r) - s_prev_2;
        X_i = s_prev_1 * c_i;

        magnitude1 = sqrtf(X_r*X_r + X_i*X_i) / req.result_length;
        phase1 = wrap_pi(atan2f(X_i, X_r));

        //Goertzel Algorithm
        s_prev_1 = 0;
        s_prev_2 = 0;
        s = 0;

        for (uint16_t i = 0; i < req.result_length; i ++) {
            uint16_t sample = (uint16_t)(req.result_buffer[i]);
            s = (float)sample + (2 * c_r * s_prev_1) - s_prev_2;
            s_prev_2 = s_prev_1;
            s_prev_1 = s; 
        }

        X_r = (s_prev_1 * c_r) - s_prev_2;
        X_i = s_prev_1 * c_i;

        magnitude2 = sqrtf(X_r*X_r + X_i*X_i) / req.result_length;
        phase2 = wrap_pi(atan2f(X_i, X_r));

        uint8_t msg[128];
        int len = snprintf((char*)msg, sizeof(msg), "Frequency %f, Delta Phase: %f, Magnitude Ratio %f\r\n", f, (phase2 - phase1) * (180.0f / (float)M_PI), magnitude1 / magnitude2);
        HAL_UART_Transmit(&huart2, msg, len, HAL_MAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));

        f *= 1.25892541179; // Multiply by 10^(1/10) to increase frequency by 1 semitone
        if (f > 1000000)
            f = 1000;
    }
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();
    //MX_ADC3_Init();
    MX_DAC_Init();
    //MX_USB_OTG_HS_PCD_Init();
    MX_TIM2_Init();
    MX_USART2_UART_Init();

    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_OC_Start(&htim2, TIM_CHANNEL_1);

    measure_init();

    xTaskCreate(
        measure_test,
        "measure_test",
        1024,
        NULL,
        1,
        NULL
    );

    vTaskStartScheduler();

    while (1);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xHigherPriorityTaskWoken = osc_DMA_Handler(0);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hdac) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xHigherPriorityTaskWoken = osc_DMA_Handler(1);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (hadc->Instance == ADC1) {
        vTaskNotifyGiveFromISR(measure_task_handle, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 140;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM14 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14)
        HAL_IncTick();
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1);
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
