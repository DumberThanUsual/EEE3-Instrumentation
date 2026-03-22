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
#include "signal.h"
#include "goertzel.h"
#include "interface.h"
#include "process.h"

void SystemClock_Config(void);

void measure_test(void *pvParameters) 
{
    while (1) {
        xTaskNotifyGive(measure_task_handle);
        xSemaphoreTake(complete, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(1000));
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
    MX_DAC_Init();
    MX_TIM2_Init();
    MX_ADC3_Init();
    MX_USART1_UART_Init();
    MX_TIM3_Init();

    //Only use if 100 ohm OZ is unpopulates
    HAL_GPIO_WritePin(SOURCE_SEL_GPIO_Port, SOURCE_SEL_Pin, GPIO_PIN_SET);
    
    //HAL_GPIO_WritePin(ABB_DC_ISO_GPIO_Port, ABB_DC_ISO_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_ACEN_GPIO_Port, IAMP_ACEN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_AC100_GPIO_Port, IAMP_AC100_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_AC1K_GPIO_Port, IAMP_AC1K_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IAMP_AC10K_GPIO_Port, IAMP_AC10K_Pin, GPIO_PIN_SET);

    buffered_new_setup.source_frequency = 10000;
    buffered_new_setup.source_amplitude = 1000;
    buffered_new_setup.auto_current_gain = 1;
    buffered_new_setup.auto_voltage_gain = 1;
    buffered_new_setup.mutex = xSemaphoreCreateMutex();

    setup.mutex = xSemaphoreCreateMutex();

    measure_init();
    signal_init();
    //interface_init();

    xTaskCreate(measure_test,"measure_test",1024,NULL,1,NULL);

    HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, DAC_MID);

    vTaskStartScheduler();

    while (1);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xHigherPriorityTaskWoken = signal_DMA_Handler(0);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hdac) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xHigherPriorityTaskWoken = signal_DMA_Handler(1);
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
