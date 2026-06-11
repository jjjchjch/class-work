/**
 ****************************************************************************************************
 * @file        dac.c
 * @brief       DAC 锯齿波输出驱动 (TIM5 定时触发 + DMA)
 *
 *              PA4 (DAC_OUT1) 输出锯齿波:
 *              - TIM5 每 1ms 产生 TRGO 信号
 *              - DAC CH1 由 TIM5 TRGO 硬件触发转换
 *              - DMA 循环传输锯齿波查找表到 DAC 数据寄存器
 *              - 输出: 0V → 3.3V 线性锯齿波, 周期 = 256 × 1ms = 256ms
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB1 = 16MHz (TIM5 所在总线)
 ****************************************************************************************************
 */

#include "dac.h"
#include "main.h"

/* ---- 锯齿波查找表 (256 步, 0 → 4080, 步长 16) ---- */
volatile uint16_t g_dac_ramp_table[DAC_RAMP_STEPS];

static DAC_HandleTypeDef  DAC_Handler;    /* DAC 句柄 */
static DMA_HandleTypeDef  DMA_Handler;    /* DMA1_Stream5 句柄 */

/**
 * @brief       DAC 锯齿波初始化 (TIM5 TRGO 触发 + DMA 循环)
 *
 *              1. 生成锯齿波查找表 (256步, 0→4080)
 *              2. 配置 PA4 为模拟输出
 *              3. 配置 DAC CH1: TIM5 TRGO 触发, 12位右对齐
 *              4. 配置 DMA 循环传输查找表到 DAC_DHR12R1
 *              5. 配置并启动 TIM5 (1ms TRGO)
 *
 * @param       无
 * @retval      无
 */
void DAC_Sawtooth_Init(void)
{
    uint16_t i;

    /* ========== 1. 生成锯齿波查找表 ========== */
    for (i = 0; i < DAC_RAMP_STEPS; i++)
    {
        g_dac_ramp_table[i] = (uint16_t)(((uint32_t)i * DAC_RAMP_MAX) / (DAC_RAMP_STEPS - 1));
    }

    /* ========== 2. 配置 PA4 为模拟输出 (DAC_OUT1) ========== */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ========== 3. 配置 DAC CH1 ========== */
    __HAL_RCC_DAC_CLK_ENABLE();

    DAC_Handler.Instance = DAC;

    if (HAL_DAC_Init(&DAC_Handler) != HAL_OK)
    {
        Error_Handler();
    }

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger      = DAC_TRIGGER_T5_TRGO;      /* TIM5 TRGO 触发 */
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;  /* 使能输出缓冲 */

    if (HAL_DAC_ConfigChannel(&DAC_Handler, &sConfig, DAC_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    /* ========== 4. 配置 DMA1_Stream5_Channel7 (内存→DAC) ========== */
    __HAL_RCC_DMA1_CLK_ENABLE();

    DMA_Handler.Instance                 = DMA1_Stream5;
    DMA_Handler.Init.Channel             = DMA_CHANNEL_7;
    DMA_Handler.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    DMA_Handler.Init.PeriphInc           = DMA_PINC_DISABLE;          /* 外设地址固定 (DAC_DHR12R1) */
    DMA_Handler.Init.MemInc              = DMA_MINC_ENABLE;           /* 内存地址递增 (查表) */
    DMA_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    DMA_Handler.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    DMA_Handler.Init.Mode                = DMA_CIRCULAR;              /* 循环模式: 表尾回到表头 */
    DMA_Handler.Init.Priority            = DMA_PRIORITY_HIGH;

    if (HAL_DMA_Init(&DMA_Handler) != HAL_OK)
    {
        Error_Handler();
    }

    /* 关联 DMA 到 DAC */
    __HAL_LINKDMA(&DAC_Handler, DMA_Handle1, DMA_Handler);

    /* ========== 5. 配置并启动 TIM5 (1ms TRGO) ========== */
    /*
     * APB1 = 16MHz, TIM5 时钟 = 16MHz
     * PSC = 15     → 计数时钟 = 16MHz / 16 = 1MHz (1us/tick)
     * ARR = 999    → 溢出周期 = (999+1) × 1us = 1ms
     *
     * MMS[2:0] = 010: 更新事件作为 TRGO 输出
     *   → DAC 收到 TIM5 TRGO 自动触发一次转换
     *   → DMA 自动将查表下一个值送入 DAC_DHR12R1
     */
    __HAL_RCC_TIM5_CLK_ENABLE();

    TIM5->PSC  = 15U;                           /* 16MHz / 16 = 1MHz           */
    TIM5->ARR  = 999U;                          /* 1MHz / 1000 = 1kHz (1ms)    */
    TIM5->EGR  = TIM_EGR_UG;                   /* 立即加载 PSC/ARR             */

    /* MMS[2:0] = 010: 更新事件作为 TRGO 输出 */
    TIM5->CR2  &= ~TIM_CR2_MMS;
    TIM5->CR2  |= TIM_TRGO_UPDATE;

    TIM5->SR    = 0U;                           /* 清除中断标志                 */
    TIM5->DIER  = 0U;                           /* 不使能中断 (仅 TRGO 输出)    */
    TIM5->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;  /* 自动重装载 + 启动            */

    /* ========== 6. 启动 DAC DMA ========== */
    if (HAL_DAC_Start_DMA(&DAC_Handler, DAC_CHANNEL_1,
                          (uint32_t *)g_dac_ramp_table,
                          DAC_RAMP_STEPS,
                          DAC_ALIGN_12B_R) != HAL_OK)
    {
        Error_Handler();
    }
}
