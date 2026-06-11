/**
 ****************************************************************************************************
 * @file        dac.c
 * @brief       DAC 波形发生器驱动 (TIM6 定时触发 + PA5 输出)
 *
 *              参考: 正点原子 实验9_2 定时触发DAC实验
 *
 *              PA5 (DAC_OUT2) 输出波形:
 *              - TIM6 产生 TRGO + 更新中断
 *              - DAC CH2 由 TIM6 TRGO 硬件触发转换
 *              - 在中断回调中更新 DAC 值 → 下一个 TRGO 锁存输出
 *              - 支持噪声波 / 三角波, 按键切换
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB1 = 16MHz, TIM6 CLK = 16MHz
 ****************************************************************************************************
 */

#include "dac.h"
#include "main.h"

/* ---- 三角波查找表 (256步, 上升128步 + 下降128步) ---- */
static uint16_t g_tri_table[DAC_TRI_STEPS];

/* ---- 全局变量 ---- */
static DAC_HandleTypeDef  DAC_Handler;    /* DAC 句柄 */
static TIM_HandleTypeDef  TIM6_Handler;   /* TIM6 句柄 */
static volatile WaveType_t g_wave_type = WAVE_TRIANGLE;  /* 当前波形类型 */
static volatile uint16_t   g_dac_index = 0;              /* 波形查表索引 */
static uint16_t            g_noise_val  = 0;             /* 噪声波当前值 */
static uint32_t            g_lfsr       = 0xABCD1234;    /* LFSR 种子 (噪声生成) */

/**
 * @brief       生成三角波查找表
 *              - 前半段: 0 → 4095 线性上升 (128步)
 *              - 后半段: 4095 → 0 线性下降 (128步)
 * @param       无
 * @retval      无
 */
static void DAC_GenTriTable(void)
{
    uint16_t i;
    uint16_t half = DAC_TRI_STEPS / 2;  /* 128 */

    for (i = 0; i < half; i++)
    {
        /* 上升段: 0 → 4095 */
        g_tri_table[i] = (uint16_t)(((uint32_t)i * DAC_TRI_MAX) / (half - 1));
    }
    for (i = half; i < DAC_TRI_STEPS; i++)
    {
        /* 下降段: 4095 → 0 */
        g_tri_table[i] = (uint16_t)(((uint32_t)(DAC_TRI_STEPS - 1 - i) * DAC_TRI_MAX) / (half - 1));
    }
}

/**
 * @brief       LFSR 伪随机数生成 (用于噪声波)
 *              16位 Galois LFSR, 多项式: x^16 + x^14 + x^13 + x^11 + 1
 * @param       无
 * @retval      12位随机值 (0~4095)
 */
static uint16_t DAC_NoiseRand(void)
{
    uint16_t bit;
    uint8_t  i;

    for (i = 0; i < 4; i++)  /* 每个TIM周期迭代4次以增加随机性 */
    {
        bit = ((g_lfsr >> 0) ^ (g_lfsr >> 2) ^ (g_lfsr >> 3) ^ (g_lfsr >> 5)) & 1;
        g_lfsr = (g_lfsr >> 1) | (bit << 15);
    }

    return (uint16_t)(g_lfsr & 0x0FFF);  /* 取低12位, 0~4095 */
}

/**
 * @brief       DAC 波形发生器初始化
 *
 *              1. 生成三角波查找表
 *              2. 配置 PA5 为模拟输出 (DAC_OUT2)
 *              3. 配置 DAC CH2: TIM6 TRGO 触发
 *              4. 配置 TIM6: 25kHz 更新率, TRGO + 中断
 *              5. 启动 DAC 通道2
 *
 * @param       无
 * @retval      无
 */
void DAC_WaveGen_Init(void)
{
    /* ========== 1. 生成三角波查找表 ========== */
    DAC_GenTriTable();

    /* ========== 2. 配置 PA5 为模拟输出 (DAC_OUT2) ========== */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_5;           /* PA5 = DAC_OUT2 */
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ========== 3. 配置 DAC CH2 (TIM6 TRGO 触发) ========== */
    __HAL_RCC_DAC_CLK_ENABLE();

    DAC_Handler.Instance = DAC;

    if (HAL_DAC_Init(&DAC_Handler) != HAL_OK)
    {
        Error_Handler();
    }

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger      = DAC_TRIGGER_T6_TRGO;      /* TIM6 TRGO 触发 */
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;  /* 使能输出缓冲 */

    if (HAL_DAC_ConfigChannel(&DAC_Handler, &sConfig, DAC_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    /* 启动 DAC 通道2 */
    if (HAL_DAC_Start(&DAC_Handler, DAC_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    /* ========== 4. 配置 TIM6 (25kHz 更新率, TRGO + 中断) ========== */
    /*
     * APB1 = 16MHz, TIM6 时钟 = 16MHz
     * PSC = 15   → 计数时钟 = 16MHz / 16 = 1MHz (1us/tick)
     * ARR = 39   → 溢出周期 = (39+1) × 1us = 40us → 25kHz
     *
     * TRGO = 更新事件 → 每次溢出触发 DAC 转换
     * 中断使能 → 在回调中更新下一个 DAC 值
     */
    __HAL_RCC_TIM6_CLK_ENABLE();

    TIM6_Handler.Instance               = TIM6;
    TIM6_Handler.Init.Prescaler         = DAC_TIM6_PSC;           /* 15 */
    TIM6_Handler.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TIM6_Handler.Init.Period            = DAC_TIM6_ARR;           /* 39 */
    TIM6_Handler.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TIM6_Handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&TIM6_Handler) != HAL_OK)
    {
        Error_Handler();
    }

    /* 清除 HAL_TIM_Base_Init 产生的更新标志 */
    __HAL_TIM_CLEAR_FLAG(&TIM6_Handler, TIM_FLAG_UPDATE);

    /* 配置 TIM6 TRGO 为更新事件 */
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(&TIM6_Handler, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /* 启动 TIM6 基础定时 + 中断 */
    if (HAL_TIM_Base_Start_IT(&TIM6_Handler) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief       TIM6 底层硬件初始化 (MSP)
 *              由 HAL_TIM_Base_Init() 自动调用
 * @param       htim: TIM 句柄指针
 * @retval      无
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        __HAL_RCC_TIM6_CLK_ENABLE();

        /* TIM6 和 DAC 共用中断向量 TIM6_DAC_IRQn */
        HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    }
}

/**
 * @brief       TIM6 周期溢出回调 (在 TIM6_DAC_IRQHandler 中调用)
 *
 *              每次 TIM6 溢出:
 *              1. TRGO 硬件触发 DAC 将 DHR 值锁存到 DOR 输出
 *              2. 本回调设置下一个 DHR 值, 等待下一次 TRGO
 *
 * @param       htim: TIM 句柄指针
 * @retval      无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        switch (g_wave_type)
        {
            case WAVE_NOISE:
                /* 噪声波: 每次生成新的随机值 */
                g_noise_val = DAC_NoiseRand();
                HAL_DAC_SetValue(&DAC_Handler, DAC_CHANNEL_2,
                                 DAC_ALIGN_12B_R, g_noise_val);
                break;

            case WAVE_TRIANGLE:
                /* 三角波: 查表循环 */
                HAL_DAC_SetValue(&DAC_Handler, DAC_CHANNEL_2,
                                 DAC_ALIGN_12B_R, g_tri_table[g_dac_index]);
                g_dac_index++;
                if (g_dac_index >= DAC_TRI_STEPS)
                {
                    g_dac_index = 0;
                }
                break;

            default:
                break;
        }
    }
}

/**
 * @brief       切换波形类型
 * @param       type: 波形类型 (WAVE_NOISE / WAVE_TRIANGLE)
 * @retval      无
 */
void DAC_WaveGen_SetType(WaveType_t type)
{
    g_wave_type = type;
    g_dac_index = 0;       /* 重置查表索引 */
    g_noise_val = 0;       /* 重置噪声值 */
}

/**
 * @brief       获取当前波形类型
 * @param       无
 * @retval      当前波形类型
 */
WaveType_t DAC_WaveGen_GetType(void)
{
    return g_wave_type;
}

/**
 * @brief       TIM6_DAC 中断处理函数
 *
 *              TIM6 和 DAC 共用中断向量 TIM6_DAC_IRQn
 *              调用 HAL_TIM_IRQHandler → HAL_TIM_PeriodElapsedCallback
 *
 * @param       无
 * @retval      无
 */
void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TIM6_Handler);
}
