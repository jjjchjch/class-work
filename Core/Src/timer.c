#include "timer.h"
#include "main.h"

/**
 * @brief  TIM3 PWM 初始化
 *         PA6 → TIM3_CH1 (AF2)  1kHz, 占空比 25%
 *         PA7 → TIM3_CH2 (AF2)  1kHz, 占空比 25%
 *
 *         SYSCLK = 16MHz (HSI, 无PLL)
 *         PSC = 15   → 计数时钟 = 16MHz / 16 = 1MHz（1?s/tick）
 *         ARR = 999  → 溢出周期 = (999+1) × 1?s = 1ms（1kHz）
 *         CCR = 250  → 占空比 = 250 / 1000 = 25%
 */
void TIM3_PWM_Init(void)
{
    /* ---- GPIO 配置：PA6 / PA7 → TIM3 复用推挽输出 ---- */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ---- TIM3 基础定时器配置 ---- */
    __HAL_RCC_TIM3_CLK_ENABLE();

    TIM_HandleTypeDef htim3 = {0};
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

    /* ---- 通道输出配置（PWM Mode 1） ---- */
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 250U;           /* CCR = 250 → 25% 占空比 */
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;

    /* CH1 → PA6 */
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    /* CH2 → PA7 */
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
}
