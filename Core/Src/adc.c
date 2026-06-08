/**
 ****************************************************************************************************
 * @file        adc.c
 * @brief       ADC 驱动代码 (TIM2 定时触发 + DMA)
 *
 *              光敏电阻分压 → PC1 (ADC1_IN11)
 *              TIM2 TRGO 每 100ms 触发一次 ADC 转换
 *              DMA2_Stream0_Channel0 循环传输结果到 g_adc_dma_buf
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB2 = 16MHz, ADC CLK = APB2/4 = 4MHz
 ****************************************************************************************************
 */

#include "adc.h"

/* ---- DMA 缓冲区 (DMA 循环写入) ---- */
volatile uint16_t g_adc_dma_buf  = 0;
volatile uint8_t  g_adc_new_data = 0;

static ADC_HandleTypeDef  ADC_Handler;    /* ADC1 句柄 */
static DMA_HandleTypeDef  DMA_Handler;    /* DMA2 句柄 */

/**
 * @brief       ADC 初始化 (TIM2 TRGO 触发 + DMA 循环)
 * @param       无
 * @retval      无
 */
void adc_init(void)
{
    /* ---- ADC1 基本配置 ---- */
    ADC_Handler.Instance                   = ADC1;
    ADC_Handler.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;   /* ADCCLK = 16/4 = 4MHz */
    ADC_Handler.Init.Resolution            = ADC_RESOLUTION_12B;             /* 12位精度 */
    ADC_Handler.Init.DataAlign             = ADC_DATAALIGN_RIGHT;            /* 右对齐 */
    ADC_Handler.Init.ScanConvMode          = DISABLE;                        /* 非扫描模式 */
    ADC_Handler.Init.ContinuousConvMode    = DISABLE;                        /* 单次转换 (由TIM触发) */
    ADC_Handler.Init.NbrOfConversion       = 1;                              /* 规则序列长度 = 1 */
    ADC_Handler.Init.DiscontinuousConvMode = DISABLE;
    ADC_Handler.Init.NbrOfDiscConversion   = 0;
    ADC_Handler.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T2_TRGO;   /* TIM2 TRGO 触发 */
    ADC_Handler.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING; /* 上升沿触发 */
    ADC_Handler.Init.DMAContinuousRequests = ENABLE;                         /* DMA 循环模式 */

    HAL_ADC_Init(&ADC_Handler);

    /* ---- 配置通道 ---- */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = ADC_PHOTO_CHANNEL;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

    HAL_ADC_ConfigChannel(&ADC_Handler, &sConfig);
}

/**
 * @brief       ADC 底层硬件初始化 (MSP)
 *              由 HAL_ADC_Init() 自动调用
 * @param       hadc: ADC 句柄指针
 * @retval      无
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hadc->Instance == ADC1)
    {
        /* ---- 使能时钟 ---- */
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();

        /* ---- PC1: 模拟输入 (光敏电阻分压) ---- */
        GPIO_InitStruct.Pin  = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* ---- DMA2_Stream0_Channel0: ADC1 数据流 ---- */
        DMA_Handler.Instance                 = DMA2_Stream0;
        DMA_Handler.Init.Channel             = DMA_CHANNEL_0;
        DMA_Handler.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        DMA_Handler.Init.PeriphInc           = DMA_PINC_DISABLE;
        DMA_Handler.Init.MemInc              = DMA_MINC_DISABLE;       /* 单缓冲, 不递增 */
        DMA_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        DMA_Handler.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
        DMA_Handler.Init.Mode                = DMA_CIRCULAR;           /* 循环模式 */
        DMA_Handler.Init.Priority            = DMA_PRIORITY_HIGH;

        HAL_DMA_Init(&DMA_Handler);

        /* 关联 DMA 到 ADC */
        __HAL_LINKDMA(hadc, DMA_Handle, DMA_Handler);

        /* DMA 传输完成中断 */
        HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    }
}

/**
 * @brief       启动 ADC DMA 传输 (循环模式)
 * @param       无
 * @retval      无
 */
void adc_start_dma(void)
{
    HAL_ADC_Start_DMA(&ADC_Handler, (uint32_t *)&g_adc_dma_buf, 1);
}

/**
 * @brief       ADC 值转换为电压 (V)
 * @param       adc_val: 12位 ADC 值 (0~4095)
 * @retval      电压值 (V)
 */
float adc_get_voltage_v(uint16_t adc_val)
{
    return (float)adc_val * (3.3f / 4096.0f);
}

/* ================================================================
 * DMA 传输完成中断回调
 * ================================================================ */
void DMA2_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&DMA_Handler);
}

/**
 * @brief       ADC 转换完成回调 (DMA 传输完成后 HAL 自动调用)
 * @param       hadc: ADC 句柄
 * @retval      无
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    g_adc_new_data = 1;   /* 通知主循环有新数据 */
}
