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
#include "main.h"

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

/* ================================================================
 * ADC2 定时触发批量采样 (TIM4 TRGO, 中断模式)
 *
 * 参照魔女科技 "实验8_2 定时触发ADC实验"
 *
 * 硬件: PC1 (ADC2_IN11) ← 光敏电阻分压
 *       TIM4 TRGO → ADC2 外部触发 (10ms)
 *       ADC 中断 → 回调存入软件缓冲区, 满 100 个通知主循环
 * ================================================================ */

static ADC_HandleTypeDef hadc2;

volatile uint16_t adc2_buf[ADC2_BUF_SIZE];
volatile uint32_t adc2_transfer_cnt  = 0;
volatile uint8_t  adc2_transfer_done = 0;
volatile uint8_t  adc2_new_val       = 0;
volatile uint16_t adc2_latest        = 0;
static volatile uint16_t adc2_buf_idx = 0;

/**
 * @brief  ADC2 初始化 (TIM4 TRGO 外部触发, 中断模式)
 */
void adc2_init(void)
{
    /* ---- GPIO: PC1 模拟输入 ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* ---- ADC2 配置 ---- */
    __HAL_RCC_ADC2_CLK_ENABLE();

    hadc2.Instance                   = ADC2;
    hadc2.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;
    hadc2.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc2.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc2.Init.ScanConvMode          = DISABLE;
    hadc2.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc2.Init.ContinuousConvMode    = DISABLE;
    hadc2.Init.NbrOfConversion       = 1;
    hadc2.Init.DiscontinuousConvMode = DISABLE;
    hadc2.Init.NbrOfDiscConversion   = 0;
    hadc2.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T3_TRGO;   /* EXTSEL=8 → TIM3_TRGO */
    hadc2.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc2.Init.DMAContinuousRequests = DISABLE;

    HAL_ADC_Init(&hadc2);

    /* ---- 通道 CH11 ---- */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = ADC_CHANNEL_11;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    sConfig.Offset       = 0;
    HAL_ADC_ConfigChannel(&hadc2, &sConfig);

    /* ---- ADC 中断 ---- */
    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);

    /* ---- 启动 ADC (中断模式, 等待 TIM4 TRGO) ---- */
    HAL_ADC_Start_IT(&hadc2);
}

/**
 * @brief  计算 100 个采样值的平均电压 (V)
 */
float adc2_get_average_v(void)
{
    uint32_t sum = 0;
    for (int i = 0; i < ADC2_BUF_SIZE; i++)
    {
        sum += adc2_buf[i];
    }
    return (float)sum / (float)ADC2_BUF_SIZE * (3.3f / 4096.0f);
}

/* ================================================================
 * ADC 中断回调
 * ================================================================ */

/**
 * @brief  ADC 转换完成回调 (ADC1 和 ADC2 共用)
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC2)
    {
        uint16_t val = HAL_ADC_GetValue(hadc);

        /* LED1 (PC5) 翻转 — 调试用: 闪烁=ADC触发正常 */
        HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_GPIO_PIN);

        adc2_latest  = val;
        adc2_new_val = 1;

        adc2_buf[adc2_buf_idx++] = val;
        if (adc2_buf_idx >= ADC2_BUF_SIZE)
        {
            adc2_buf_idx = 0;
            adc2_transfer_cnt++;
            adc2_transfer_done = 1;
        }
    }
    else if (hadc->Instance == ADC1)
    {
        /* ADC1 定时触发采样: 由 main.c 中的回调逻辑处理 */
        extern void ADC1_ConvCpltCallback(uint16_t val);
        ADC1_ConvCpltCallback(HAL_ADC_GetValue(hadc));
    }
}

/**
 * @brief  ADC 全局中断处理
 *         STM32F407: ADC1/ADC2/ADC3 共享此中断向量
 */
void ADC_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc2);
    extern ADC_HandleTypeDef hadc1;
    HAL_ADC_IRQHandler(&hadc1);
}
