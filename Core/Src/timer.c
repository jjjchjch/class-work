#include "timer.h"
#include "main.h"

/* ---- 全局变量 ---- */
TIM_HandleTypeDef htim3 = {0};
volatile TIM3_IC_Result_t g_ic_result = {0};

/**
 * @brief  TIM3 PWM 初始化
 *         PA6 -> TIM3_CH1 (AF2)  1kHz, 占空比 50%
 *
 *         SYSCLK = 16MHz (HSI, 无PLL)
 *         PSC = 15   -> 计数时钟 = 16MHz / 16 = 1MHz（1us/tick）
 *         ARR = 999  -> 溢出周期 = (999+1) x 1us = 1ms（1kHz）
 *         CCR1 = 500 -> 占空比 = 500 / 1000 = 50%  (PA6)
 */
void TIM3_PWM_Init(void)
{
    /* ---- GPIO: PA6 -> TIM3_CH1 复用推挽输出 ---- */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ---- TIM3 基础定时器配置 ---- */
    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 15U;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = 999U;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    /* ---- CH1: PA6, 50% 占空比 ---- */
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 500U;           /* CCR = 500 -> 50% */
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

/**
 * @brief  TIM3 输入捕获初始化 (CH2: PA7)
 *
 *         PA7 -> TIM3_CH2 (AF2)  输入捕获
 *         双沿捕获: 上升沿 + 下降沿
 *         用于测量 PA6 输出的 PWM 波形
 *         （需用杜邦线将 PA6 与 PA7 短接）
 *
 *         捕获参数通过 TIM3_IRQHandler 计算
 *         结果存储在 g_ic_result 中
 */
void TIM3_IC_Init(void)
{
    /* ---- GPIO: PA7 -> TIM3_CH2 复用输入 ---- */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ---- 配置 TIM3 CH2 为输入捕获（双沿）---- */
    TIM_IC_InitTypeDef sConfigIC = {0};
    sConfigIC.ICPolarity  = TIM_ICPOLARITY_RISING;   /* 初始上升沿捕获   */
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI; /* CC2 直连 TI2    */
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;           /* 不分频          */
    sConfigIC.ICFilter    = 0x0F;                      /* 滤波器: 8次采样 */

    if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    /* ---- 使能 TIM3 捕获/更新中断 ---- */
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_CC2);   /* CH2 捕获中断 */
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE); /* 溢出更新中断 */

    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);     /* 最高优先级 */
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    /* 启动 CH2 输入捕获 */
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
}

/**
 * @brief  TIM4 软件PWM呼吸灯初始化 (PB1 / LED2, 低电平点亮)
 *
 *         中断频率 = 16MHz / (PSC+1) / (ARR+1)
 *                  = 16MHz / 16 / 50 = 20kHz (每50?s一次中断)
 *
 *         软件PWM: 100步 → PWM频率 = 20kHz/100 = 200Hz（无闪烁）
 *         每2个PWM周期(10ms)更新一次占空比
 *         0→99→0 共199步 × 10ms ≈ 2s呼吸周期
 */
void TIM4_Breathe_Init(void)
{
    __HAL_RCC_TIM4_CLK_ENABLE();

    TIM4->PSC  = 15U;                         /* 16MHz / 16 = 1MHz         */
    TIM4->ARR  = 49U;                         /* 1MHz / 50  = 20kHz        */
    TIM4->EGR  = TIM_EGR_UG;                 /* 立即加载PSC/ARR           */
    TIM4->SR   = 0U;                          /* 清除所有中断标志          */
    TIM4->DIER = TIM_DIER_UIE;               /* 使能更新中断              */
    TIM4->CR1  = TIM_CR1_ARPE | TIM_CR1_CEN; /* 自动重装载 + 启动计数器  */

    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
}
