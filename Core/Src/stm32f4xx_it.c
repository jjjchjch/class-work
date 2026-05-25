/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "encoder.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/**
 * @brief  EXTI9_5 中断处理函数 — EC11 CLK(PA7) 下降沿
 *         NVIC: EXTI9_5_IRQn，抢占优先级 1（最高）
 *
 *         CLK 下降沿时读取 DT(PA6) 判断旋转方向:
 *           DT = 1 (HIGH) → 正转 → 切换 LED1 (PC5)
 *           DT = 0 (LOW)  → 逆转 → 切换 LED2 (PB1)
 */
void EXTI9_5_IRQHandler(void)
{
    static uint32_t last_tick = 0;

    if (__HAL_GPIO_EXTI_GET_IT(ENC_CLK_GPIO_PIN))
    {
        if (HAL_GetTick() - last_tick > 5U)   /* 消抖: 5 ms（编码器消抖窗口小） */
        {
            last_tick = HAL_GetTick();
            if (HAL_GPIO_ReadPin(ENC_DT_GPIO_PORT, ENC_DT_GPIO_PIN) == GPIO_PIN_SET)
            {
                HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_GPIO_PIN);  /* 正转 → LED1 */
            }
            else
            {
                HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_GPIO_PIN);  /* 逆转 → LED2 */
            }
        }
        __HAL_GPIO_EXTI_CLEAR_IT(ENC_CLK_GPIO_PIN);
    }
}

/**
 * @brief  EXTI4 中断处理函数 — EC11 SW(PC4) 按下
 *         NVIC: EXTI4_IRQn，抢占优先级 2（次级）
 *         CLK ISR 可抢占本 ISR；本 ISR 不能抢占 CLK ISR
 *
 *         SW 按下（低电平）→ 切换 LED3 (PB2)
 */
void EXTI4_IRQHandler(void)
{
    static uint32_t last_tick = 0;

    if (__HAL_GPIO_EXTI_GET_IT(ENC_SW_GPIO_PIN))
    {
        if (HAL_GetTick() - last_tick > 200U)   /* 消抖: 200 ms */
        {
            last_tick = HAL_GetTick();
            HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_GPIO_PIN);      /* 按下  → LED3 */
        }
        __HAL_GPIO_EXTI_CLEAR_IT(ENC_SW_GPIO_PIN);
    }
}

/* USER CODE END 1 */
