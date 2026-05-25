/**
 ****************************************************************************************************
 * EC11 旋转编码器初始化
 *
 *  CLK (PA7): 下降沿中断 → EXTI9_5_IRQn, 抢占优先级 1
 *  DT  (PA6): 普通输入，在 CLK ISR 内读取判方向
 *  SW  (PC4): 下降沿中断 → EXTI4_IRQn,   抢占优先级 2
 ****************************************************************************************************
 */

#include "encoder.h"

/**
 * @brief  EC11 编码器外部中断初始化
 * @retval 无
 */
void encoder_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    ENC_CLK_GPIO_CLK_EN();
    ENC_DT_GPIO_CLK_EN();
    ENC_SW_GPIO_CLK_EN();

    /* CLK: PA7 — 上拉输入，下降沿触发中断（旋转检测） */
    gpio_init.Pin   = ENC_CLK_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_IT_FALLING;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ENC_CLK_GPIO_PORT, &gpio_init);

    /* DT: PA6 — 上拉输入，普通读取，不触发中断 */
    gpio_init.Pin   = ENC_DT_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ENC_DT_GPIO_PORT, &gpio_init);

    /* SW: PC4 — 上拉输入，下降沿触发中断（按键检测） */
    gpio_init.Pin   = ENC_SW_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_IT_FALLING;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ENC_SW_GPIO_PORT, &gpio_init);

    /* NVIC 优先级配置
     *  EXTI9_5_IRQn (CLK/PA7) — 抢占优先级 1: 旋转检测，时序最敏感，优先级最高
     *  EXTI4_IRQn   (SW/PC4)  — 抢占优先级 2: 按键检测，次优先级
     *  CLK ISR 可抢占 SW ISR；SW ISR 不能抢占 CLK ISR
     */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}
