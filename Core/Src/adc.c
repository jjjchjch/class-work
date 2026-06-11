/**
 ****************************************************************************************************
 * @file        adc.c
 * @brief       ADC 驱动 (单次转换 + 轮询, 最简可靠方案)
 *
 *              光敏电阻分压 → PC1 (ADC1_IN11)
 *              adc_read() 启动软件触发转换 → 等待 EOC → 返回结果
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB2 = 16MHz, ADC CLK = APB2/4 = 4MHz
 ****************************************************************************************************
 */

#include "adc.h"

static ADC_HandleTypeDef ADC_Handler;    /* ADC1 句柄 */

/**
 * @brief       ADC 初始化 (单通道, 软件触发)
 *
 *              配置 PC1 为模拟输入, ADC1_CH11
 *              软件触发, 单次转换, 无 DMA, 无中断
 *
 * @param       无
 * @retval      无
 */
void adc_init(void)
{
    /* ---- GPIO: PC1 模拟输入 ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* ---- ADC1 基本配置 ---- */
    __HAL_RCC_ADC1_CLK_ENABLE();

    ADC_Handler.Instance                   = ADC1;
    ADC_Handler.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;   /* ADCCLK = 4MHz */
    ADC_Handler.Init.Resolution            = ADC_RESOLUTION_12B;
    ADC_Handler.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    ADC_Handler.Init.ScanConvMode          = DISABLE;
    ADC_Handler.Init.ContinuousConvMode    = DISABLE;                        /* 单次转换 */
    ADC_Handler.Init.NbrOfConversion       = 1;
    ADC_Handler.Init.DiscontinuousConvMode = DISABLE;
    ADC_Handler.Init.NbrOfDiscConversion   = 0;
    ADC_Handler.Init.ExternalTrigConv      = ADC_SOFTWARE_START;             /* 软件触发 */
    ADC_Handler.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;

    HAL_ADC_Init(&ADC_Handler);

    /* ---- 配置通道 ---- */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = ADC_PHOTO_CHANNEL;         /* ADC_CHANNEL_11 */
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;  /* 长采样时间, 高阻抗信号稳定 */

    HAL_ADC_ConfigChannel(&ADC_Handler, &sConfig);

    /* ==== 显式寄存器写入, 确保万无一失 ==== */

    /* 规则序列长度 = 1 (SQR1[23:20] = 0) */
    ADC1->SQR1 = 0;

    /* 规则序列第1个转换 = CH11 (SQR3[4:0] = 11) */
    ADC1->SQR3 = 11;

    /* 采样时间 CH11 = 480 周期 (SMPR1[15:12] = 0b111 = 480 cycles) */
    ADC1->SMPR1 |= (0x7U << 12);   /* 480 cycles for channel 11 */
}

/**
 * @brief       单次 ADC 转换 (直接寄存器操作, 绕过 HAL)
 *
 *              软件触发 → 等待 EOC → 读 DR → 返回
 *
 * @param       无
 * @retval      12位 ADC 值 (0~4095)
 */
uint16_t adc_read(void)
{
    /* 确保 ADC 已使能, 若未使能则使能并等待稳定 */
    if (!(ADC1->CR2 & ADC_CR2_ADON))
    {
        ADC1->CR2 |= ADC_CR2_ADON;
        /* 等待 ADC 稳定 (tSTAB ≈ 几微秒, 这里等几个 APB2 周期) */
        for (volatile uint32_t i = 0; i < 100; i++);
    }

    /* 清除 EOC 标志 (写 0 清除, 或读 DR 清除 — 保险起见先读一次) */
    (void)ADC1->DR;

    /* 软件触发启动转换 */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    /* 等待转换完成 */
    uint32_t timeout = 1000000;
    while (!(ADC1->SR & ADC_SR_EOC))
    {
        if (--timeout == 0) return 0;
    }

    /* 读取结果 (读 DR 自动清除 EOC) */
    return (uint16_t)(ADC1->DR & 0x0FFF);
}

/**
 * @brief       ADC 值 → 电压 (V)
 */
float adc_get_voltage_v(uint16_t adc_val)
{
    return (float)adc_val * (3.3f / 4096.0f);
}
