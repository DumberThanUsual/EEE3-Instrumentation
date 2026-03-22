/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define IAMP_ACEN_Pin GPIO_PIN_13
#define IAMP_ACEN_GPIO_Port GPIOC
#define IAMP_AC10K_Pin GPIO_PIN_14
#define IAMP_AC10K_GPIO_Port GPIOC
#define IAMP_AC1K_Pin GPIO_PIN_15
#define IAMP_AC1K_GPIO_Port GPIOC
#define IAMP_AC100_Pin GPIO_PIN_0
#define IAMP_AC100_GPIO_Port GPIOC
#define IAMP_OUT_Pin GPIO_PIN_1
#define IAMP_OUT_GPIO_Port GPIOC
#define VAMP_AC1K_Pin GPIO_PIN_2
#define VAMP_AC1K_GPIO_Port GPIOC
#define IB_MON_Pin GPIO_PIN_3
#define IB_MON_GPIO_Port GPIOC
#define VAMP_AC100_Pin GPIO_PIN_0
#define VAMP_AC100_GPIO_Port GPIOA
#define VAMP_AC10K_Pin GPIO_PIN_1
#define VAMP_AC10K_GPIO_Port GPIOA
#define VAMP_ACEN_Pin GPIO_PIN_2
#define VAMP_ACEN_GPIO_Port GPIOA
#define VAMP_OUT_Pin GPIO_PIN_3
#define VAMP_OUT_GPIO_Port GPIOA
#define SOURCE_SIGNAL_Pin GPIO_PIN_4
#define SOURCE_SIGNAL_GPIO_Port GPIOA
#define SOURCE_BIAS_Pin GPIO_PIN_5
#define SOURCE_BIAS_GPIO_Port GPIOA
#define VAMP_nMUL1_Pin GPIO_PIN_6
#define VAMP_nMUL1_GPIO_Port GPIOA
#define VAMP_DIV4_Pin GPIO_PIN_7
#define VAMP_DIV4_GPIO_Port GPIOA
#define VAMP_nMUL3_Pin GPIO_PIN_4
#define VAMP_nMUL3_GPIO_Port GPIOC
#define VAMP_nMUL100_Pin GPIO_PIN_5
#define VAMP_nMUL100_GPIO_Port GPIOC
#define SOURCE_nMUL4_Pin GPIO_PIN_0
#define SOURCE_nMUL4_GPIO_Port GPIOB
#define SOURCE_DIV10_Pin GPIO_PIN_1
#define SOURCE_DIV10_GPIO_Port GPIOB
#define SOURCE_SEL_Pin GPIO_PIN_2
#define SOURCE_SEL_GPIO_Port GPIOB
#define DAC_7_Pin GPIO_PIN_7
#define DAC_7_GPIO_Port GPIOC
#define DAC_6_Pin GPIO_PIN_8
#define DAC_6_GPIO_Port GPIOC
#define DAC_5_Pin GPIO_PIN_9
#define DAC_5_GPIO_Port GPIOC
#define DAC_4_Pin GPIO_PIN_8
#define DAC_4_GPIO_Port GPIOA
#define DAC_3_Pin GPIO_PIN_9
#define DAC_3_GPIO_Port GPIOA
#define DAC_2_Pin GPIO_PIN_10
#define DAC_2_GPIO_Port GPIOA
#define DAC_1_Pin GPIO_PIN_11
#define DAC_1_GPIO_Port GPIOA
#define DAC_0_Pin GPIO_PIN_12
#define DAC_0_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define RANGE_10_Pin GPIO_PIN_15
#define RANGE_10_GPIO_Port GPIOA
#define RANGE_100_Pin GPIO_PIN_10
#define RANGE_100_GPIO_Port GPIOC
#define ABB_DC_ISO_Pin GPIO_PIN_11
#define ABB_DC_ISO_GPIO_Port GPIOC
#define RANGE_100K_Pin GPIO_PIN_12
#define RANGE_100K_GPIO_Port GPIOC
#define RANGE_10K_Pin GPIO_PIN_2
#define RANGE_10K_GPIO_Port GPIOD
#define RANGE_1K_Pin GPIO_PIN_3
#define RANGE_1K_GPIO_Port GPIOB
#define IAMP_nMUL1_Pin GPIO_PIN_4
#define IAMP_nMUL1_GPIO_Port GPIOB
#define IAMP_nMUL3_Pin GPIO_PIN_5
#define IAMP_nMUL3_GPIO_Port GPIOB
#define IAMP_DIV4_Pin GPIO_PIN_8
#define IAMP_DIV4_GPIO_Port GPIOB
#define IAMP_nMUL100_Pin GPIO_PIN_9
#define IAMP_nMUL100_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
