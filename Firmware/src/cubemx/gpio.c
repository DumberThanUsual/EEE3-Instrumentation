/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
     PB14   ------> USB_OTG_HS_DM
     PB15   ------> USB_OTG_HS_DP
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, IAMP_ACEN_Pin|IAMP_AC10K_Pin|IAMP_AC1K_Pin|IAMP_AC100_Pin
                          |VAMP_AC1K_Pin|VAMP_nMUL3_Pin|VAMP_nMUL100_Pin|DAC_7_Pin
                          |DAC_6_Pin|DAC_5_Pin|RANGE_100_Pin|ABB_DC_ISO_Pin
                          |RANGE_100K_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, VAMP_AC100_Pin|VAMP_AC10K_Pin|VAMP_ACEN_Pin|VAMP_nMUL1_Pin
                          |VAMP_DIV4_Pin|DAC_4_Pin|DAC_3_Pin|DAC_2_Pin
                          |DAC_1_Pin|DAC_0_Pin|RANGE_10_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SOURCE_nMUL4_Pin|SOURCE_DIV10_Pin|SOURCE_SEL_Pin|RANGE_1K_Pin
                          |IAMP_nMUL1_Pin|IAMP_nMUL3_Pin|IAMP_DIV4_Pin|IAMP_nMUL100_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RANGE_10K_GPIO_Port, RANGE_10K_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : IAMP_ACEN_Pin IAMP_AC10K_Pin IAMP_AC1K_Pin IAMP_AC100_Pin
                           VAMP_AC1K_Pin VAMP_MUL3_Pin VAMP_nMUL100_Pin DAC_7_Pin
                           DAC_6_Pin DAC_5_Pin RANGE_100_Pin ABB_DC_ISO_Pin
                           RANGE_100K_Pin */
  GPIO_InitStruct.Pin = IAMP_ACEN_Pin|IAMP_AC10K_Pin|IAMP_AC1K_Pin|IAMP_AC100_Pin
                          |VAMP_AC1K_Pin|VAMP_nMUL3_Pin|VAMP_nMUL100_Pin|DAC_7_Pin
                          |DAC_6_Pin|DAC_5_Pin|RANGE_100_Pin|ABB_DC_ISO_Pin
                          |RANGE_100K_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : VAMP_AC100_Pin VAMP_AC10K_Pin VAMP_ACEN_Pin VAMP_MUL1_Pin
                           VAMP_DIV4_Pin DAC_4_Pin DAC_3_Pin DAC_2_Pin
                           DAC_1_Pin DAC_0_Pin RANGE_10_Pin */
  GPIO_InitStruct.Pin = VAMP_AC100_Pin|VAMP_AC10K_Pin|VAMP_ACEN_Pin|VAMP_nMUL1_Pin
                          |VAMP_DIV4_Pin|DAC_4_Pin|DAC_3_Pin|DAC_2_Pin
                          |DAC_1_Pin|DAC_0_Pin|RANGE_10_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SOURCE_nMUL4_Pin SOURCE_DIV10_Pin SOURCE_SEL_Pin RANGE_1K_Pin
                           IAMP_MUL1_Pin IAMP_MUL3_Pin IAMP_DIV4_Pin IAMP_nMUL100_Pin */
  GPIO_InitStruct.Pin = SOURCE_nMUL4_Pin|SOURCE_DIV10_Pin|SOURCE_SEL_Pin|RANGE_1K_Pin
                          |IAMP_nMUL1_Pin|IAMP_nMUL3_Pin|IAMP_DIV4_Pin|IAMP_nMUL100_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_OTG_HS_FS;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RANGE_10K_Pin */
  GPIO_InitStruct.Pin = RANGE_10K_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RANGE_10K_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
