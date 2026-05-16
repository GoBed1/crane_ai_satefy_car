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
#include "stm32h7xx_hal.h"

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
#define POWER_3V_Pin GPIO_PIN_6
#define POWER_3V_GPIO_Port GPIOE
#define POWER_5V_Pin GPIO_PIN_13
#define POWER_5V_GPIO_Port GPIOC
#define ETH_RST_Pin GPIO_PIN_0
#define ETH_RST_GPIO_Port GPIOC
#define LORA1_NSS_Pin GPIO_PIN_15
#define LORA1_NSS_GPIO_Port GPIOE
#define HEART_LED_Pin GPIO_PIN_10
#define HEART_LED_GPIO_Port GPIOD
#define LORA1_DIO1_Pin GPIO_PIN_3
#define LORA1_DIO1_GPIO_Port GPIOD
#define LORA1_DIO1_EXTI_IRQn EXTI3_IRQn
#define LORA1_RESET_Pin GPIO_PIN_4
#define LORA1_RESET_GPIO_Port GPIOD
#define LORA1_RF_EN_Pin GPIO_PIN_7
#define LORA1_RF_EN_GPIO_Port GPIOD
#define LORA1_BUSY_Pin GPIO_PIN_7
#define LORA1_BUSY_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define LOG_ERR(tag, ...) printf(__VA_ARGS__)
#define LOG_INFO(tag, ...) printf(__VA_ARGS__)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
