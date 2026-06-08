/**
 ****************************************************************************************************
 * @file        adc.c
 * @brief       ADC 驱动代码 (软件触发)
 *
 *              芯片内部温度 → ADC1_IN16 (内部通道)
 *              光敏电阻分压  → PC1  (ADC1_IN11)
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB2 = 16MHz, ADC CLK = APB2/4 = 4MHz
 *
 *              12位精度, 转换时间 = 采样周期 + 12 ADC周期
 *              外部通道: 15采样周期 → 27周期 = 6.75us
 *              温度通道: 480采样周期 → 492周期 = 123us
 ****************************************************************************************************
 */

#include "adc.h"
#include "delay.h"

static ADC_HandleTypeDef     ADC_Handler;    /* ADC1 句柄 */

/**
 * @brief       ADC 初始化 (ADC1, 软件触发)
 * @param       无
 * @retval      无
 */
void adc_init(void)
{
    /* ---- 使能内部温度传感器 (ADC 公共配置) ---- */
    /* STM32F407: TSVREFE 位使能内部温度传感器和 VREFINT */
    if ((ADC123_COMMON->CCR & ADC_CCR_TSVREFE) == 0U)
    {
        ADC123_COMMON->CCR |= ADC_CCR_TSVREFE;
        HAL_Delay(1);   /* 等待温度传感器稳定 (tSTART ≈ 10μs) */
    }

    /* ---- ADC1 基本配置 ---- */
    ADC_Handler.Instance                   = ADC1;
    ADC_Handler.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;   /* ADCCLK = 16/4 = 4MHz */
    ADC_Handler.Init.Resolution            = ADC_RESOLUTION_12B;             /* 12位精度 */
    ADC_Handler.Init.DataAlign             = ADC_DATAALIGN_RIGHT;            /* 右对齐 */
    ADC_Handler.Init.ScanConvMode          = DISABLE;                        /* 非扫描模式, 单通道 */
    ADC_Handler.Init.ContinuousConvMode    = DISABLE;                        /* 单次转换 */
    ADC_Handler.Init.NbrOfConversion       = 1;                              /* 规则序列长度 = 1 */
    ADC_Handler.Init.DiscontinuousConvMode = DISABLE;                        /* 禁止不连续模式 */
    ADC_Handler.Init.NbrOfDiscConversion   = 0;
    ADC_Handler.Init.ExternalTrigConv      = ADC_SOFTWARE_START;             /* 软件触发 */
    ADC_Handler.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;  /* 无外部触发边沿 */
    ADC_Handler.Init.DMAContinuousRequests = DISABLE;                        /* 不使用DMA */

    HAL_ADC_Init(&ADC_Handler);
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

        /* ---- PC1: 模拟输入 (光敏电阻分压) ---- */
        /* 内部温度传感器 ADC1_IN16 无需 GPIO 配置 */
        GPIO_InitStruct.Pin  = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
}

/**
 * @brief       获取指定通道的单次 ADC 转换结果
 * @param       ch: ADC 通道号
 * @retval      12位转换结果 (0~4095)
 */
uint32_t adc_get_result(uint32_t ch)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* ---- 配置转换通道 ---- */
    sConfig.Channel      = ch;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = (ch == ADC_CHANNEL_TEMPSENSOR)
                           ? ADC_SAMPLETIME_480CYCLES   /* 温度传感器需要较长采样时间 */
                           : ADC_SAMPLETIME_15CYCLES;   /* 外部通道: 15周期 */

    HAL_ADC_ConfigChannel(&ADC_Handler, &sConfig);

    /* ---- 启动转换并等待完成 ---- */
    HAL_ADC_Start(&ADC_Handler);
    HAL_ADC_PollForConversion(&ADC_Handler, 10);        /* 10ms 超时 */

    return (uint16_t)HAL_ADC_GetValue(&ADC_Handler);
}

/**
 * @brief       获取指定通道多次转换的平均值
 * @param       ch   : ADC 通道号
 * @param       times: 采样次数
 * @retval      平均转换值 (0~4095)
 */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times)
{
    uint32_t sum = 0;
    uint8_t i;

    for (i = 0; i < times; i++)
    {
        sum += adc_get_result(ch);
        delay_ms(5);
    }

    return sum / times;
}

/**
 * @brief       获取通道电压值 (V)
 * @param       ch   : ADC 通道号
 * @param       times: 平均采样次数
 * @retval      电压值 (V), 参考电压 3.3V
 */
float adc_get_voltage(uint32_t ch, uint8_t times)
{
    uint32_t adcx = adc_get_result_average(ch, times);
    return (float)adcx * (3.3f / 4096.0f);
}

/**
 * @brief       获取通道电压值 (mV, 整数)
 * @param       ch   : ADC 通道号
 * @param       times: 平均采样次数
 * @retval      电压值 (mV), 0~3300
 */
uint32_t adc_get_voltage_mv(uint32_t ch, uint8_t times)
{
    uint32_t adcx = adc_get_result_average(ch, times);
    return (adcx * 3300UL) / 4096UL;
}

/**
 * @brief       获取芯片内部温度 (°C)
 *
 *              STM32F407 温度传感器特性 (datasheet):
 *                V25       = 0.76V   (25°C 时的电压)
 *                Avg_Slope = 2.5mV/°C
 *
 *              公式: Temperature = ((V25 - V_SENSE) / Avg_Slope) + 25
 *
 * @param       times: 平均采样次数
 * @retval      芯片温度 (°C)
 */
float adc_get_temperature(uint8_t times)
{
    float v_sense;
    float temp;

    v_sense = adc_get_voltage(ADC_CHANNEL_TEMPSENSOR, times);

    /* 温度传感器: V_SENSE 随温度升高而升高 (正温度系数) */
    temp = ((v_sense - 0.76f) / 0.0025f) + 25.0f;

    return temp;
}
