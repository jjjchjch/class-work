/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    time.c
  * @brief   Time keeping and timer initialization
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
#include "time.h"
#include "main.h"
#include "uart.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define TIM_PRESCALER_1KHZ 15999U
#define TIM4_PERIOD_250MS  249U
#define TIM5_PERIOD_1S     999U

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;

static volatile uint8_t g_tim4_elapsed = 0U;
static volatile uint8_t g_tim5_elapsed = 0U;
static volatile uint8_t g_hours = 0U;
static volatile uint8_t g_minutes = 0U;
static volatile uint8_t g_seconds = 0U;

/* Private function prototypes -----------------------------------------------*/
static void MX_TIM4_Init(void);
static void MX_TIM5_Init(void);

/**
  * @brief  Initialize all timer related functions
  * @param  None
  * @retval None
  */
void Time_Init(void)
{
  MX_TIM4_Init();
  MX_TIM5_Init();
  TimeKeeper_Init();
}

/**
  * @brief  Initialize time keeper variables
  * @param  None
  * @retval None
  */
void TimeKeeper_Init(void)
{
  g_hours = 0U;
  g_minutes = 0U;
  g_seconds = 0U;
}

/**
  * @brief  Increment time by one second
  * @param  None
  * @retval None
  */
void TimeKeeper_Tick(void)
{
  g_seconds++;
  if (g_seconds >= 60U)
  {
    g_seconds = 0U;
    g_minutes++;
    if (g_minutes >= 60U)
    {
      g_minutes = 0U;
      g_hours++;
      if (g_hours >= 24U)
      {
        g_hours = 0U;
      }
    }
  }
}

/**
  * @brief  Print current time via UART
  * @param  None
  * @retval None
  */
void Print_CurrentTime(void)
{
  char message[32];
  int length = snprintf(message,
                        sizeof(message),
                        "Current time: %02u:%02u:%02u\r\n",
                        (unsigned int)g_hours,
                        (unsigned int)g_minutes,
                        (unsigned int)g_seconds);

  if (length > 0)
  {
    UART1_SendString(message);
  }
}

/**
  * @brief  Initialize TIM4 for 250ms timing
  * @param  None
  * @retval None
  */
static void MX_TIM4_Init(void)
{
  __HAL_RCC_TIM4_CLK_ENABLE();

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = TIM_PRESCALER_1KHZ;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = TIM4_PERIOD_250MS;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

/**
  * @brief  Initialize TIM5 for 1s timing
  * @param  None
  * @retval None
  */
static void MX_TIM5_Init(void)
{
  __HAL_RCC_TIM5_CLK_ENABLE();

  htim5.Instance = TIM5;
  htim5.Init.Prescaler = TIM_PRESCALER_1KHZ;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = TIM5_PERIOD_1S;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(TIM5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM5_IRQn);
}

/**
  * @brief  Timer period elapsed callback
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
  {
    g_tim4_elapsed = 1U;
  }
  else if (htim->Instance == TIM5)
  {
    g_tim5_elapsed = 1U;
  }
}

/**
  * @brief  Get TIM4 elapsed flag
  * @param  None
  * @retval Flag value
  */
uint8_t TIM4_GetElapsed(void)
{
  uint8_t tmp = g_tim4_elapsed;
  g_tim4_elapsed = 0U;
  return tmp;
}

/**
  * @brief  Get TIM5 elapsed flag
  * @param  None
  * @retval Flag value
  */
uint8_t TIM5_GetElapsed(void)
{
  uint8_t tmp = g_tim5_elapsed;
  g_tim5_elapsed = 0U;
  return tmp;
}

/**
  * @brief  Start all timers
  * @param  None
  * @retval None
  */
void Time_Start(void)
{
  if (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Base_Start_IT(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
}
