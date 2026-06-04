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
#include "timer.h"
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
 * @brief  TIM2 中断处理——数字钟 1 秒定时
 *
 *         每 1 秒触发一次，更新时分秒
 *         设置 g_clock_update 标志通知主循环刷新 LCD
 */
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR = ~TIM_SR_UIF;              /* 清除中断标志 */

        g_clock_sec++;
        if (g_clock_sec >= 60)
        {
            g_clock_sec = 0;
            g_clock_min++;
            if (g_clock_min >= 60)
            {
                g_clock_min = 0;
                g_clock_hour++;
                if (g_clock_hour >= 24)
                {
                    g_clock_hour = 0;
                }
            }
        }
        g_clock_update = 1;                  /* 通知主循环刷新显示 */
    }
}

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

/**
 * @brief  TIM3 中断处理——输入捕获 (CH2: PA7)
 *
 *         状态机: 上升沿 → 下降沿 → 上升沿 → 计算并更新 g_ic_result
 *         捕获 PA6 输出的 PWM 波形（需杜邦线连接 PA6—PA7）
 *
 *         同时处理 UPDATE 溢出中断，记录溢出次数以支持长周期测量
 */
void TIM3_IRQHandler(void)
{
    static uint32_t ovf_cnt     = 0;     /* 溢出次数计数器          */
    static uint32_t cap_rising1 = 0;     /* 第一次上升沿 CCR        */
    static uint32_t cap_falling = 0;     /* 下降沿 CCR              */
    static uint32_t cap_rising2 = 0;     /* 第二次上升沿 CCR        */
    static uint8_t  cap_state   = 0;     /* 0=等上升沿1, 1=等下降沿, 2=等上升沿2 */

    /* ---- 溢出更新中断：累计溢出次数 ---- */
    if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE))
    {
        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
        ovf_cnt++;
        return;
    }

    /* ---- CH2 捕获中断 ---- */
    if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC2))
    {
        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_CC2);

        uint32_t ccr = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_2);

        switch (cap_state)
        {
        case 0: /* 第一次上升沿 */
            cap_rising1 = ccr;
            ovf_cnt     = 0;
            cap_state   = 1;
            /* 切换为下降沿捕获 */
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_2, TIM_ICPOLARITY_FALLING);
            break;

        case 1: /* 下降沿 */
            cap_falling = ovf_cnt * (htim3.Init.Period + 1U) + ccr;
            cap_state   = 2;
            /* 切换回上升沿捕获 */
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_2, TIM_ICPOLARITY_RISING);
            break;

        case 2: /* 第二次上升沿 → 完成一次完整测量 */
        {
            cap_rising2 = ovf_cnt * (htim3.Init.Period + 1U) + ccr;

            /* 计算周期和高电平时间（单位：us，因为计数时钟=1MHz） */
            uint32_t period  = cap_rising2 - cap_rising1;   /* 周期 (us)   */
            uint32_t high_us = cap_falling  - cap_rising1;   /* 高电平 (us) */

            if (period > 0)
            {
                g_ic_result.period_us = period;
                g_ic_result.high_us   = high_us;
                g_ic_result.duty      = high_us * 1000U / period; /* 千分比 */
                g_ic_result.freq_hz   = 1000000U / period;         /* Hz      */
                g_ic_result.valid     = 1;
            }

            /* 重置状态，准备下一次测量 */
            cap_state = 0;
            ovf_cnt   = 0;
            break;
        }

        default:
            cap_state = 0;
            ovf_cnt   = 0;
            break;
        }
    }
}

/**
 * @brief  TIM4 更新中断——软件PWM呼吸灯 (PB1 / LED2, 低电平点亮)
 *
 *         中断频率 20kHz；软件PWM 100步 → 200Hz
 *         每 2 个PWM周期（10ms）更新一次占空比
 *         0→99→0 共199步 × 10ms ≈ 2s 呼吸周期
 */
void TIM4_IRQHandler(void)
{
    if (TIM4->SR & TIM_SR_UIF)
    {
        TIM4->SR = ~TIM_SR_UIF;              /* 清除中断标志                    */

        static uint8_t pwm_cnt    = 0U;      /* 软件PWM步数 0-99             */
        static uint8_t duty       = 0U;      /* 当前占空比  0-99             */
        static uint8_t breath_dir = 1U;      /* 1=增亮  0=变暗               */
        static uint8_t cycle_cnt  = 0U;      /* PWM完整周期计数            */

        /* PB1 (LED2) 低电平点亮 */
        if (pwm_cnt < duty)
            GPIOB->BSRR = (uint32_t)GPIO_PIN_1 << 16U;  /* 低电平: 亮 */
        else
            GPIOB->BSRR = GPIO_PIN_1;                    /* 高电平: 灯灯 */

        if (++pwm_cnt >= 100U)
        {
            pwm_cnt = 0U;
            if (++cycle_cnt >= 2U)           /* 每 2 个PWM周期 = 10ms */
            {
                cycle_cnt = 0U;
                if (breath_dir)
                {
                    if (++duty >= 99U) breath_dir = 0U;
                }
                else
                {
                    if (duty == 0U) breath_dir = 1U;
                    else            duty--;
                }
            }
        }
    }
}
