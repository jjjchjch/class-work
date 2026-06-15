/**
 ****************************************************************************************************
 * @file        dac.c
 * @brief       DAC 驱动 (参考: 正点原子 实验9_2 定时触发DAC实验)
 *
 *              PA4 (DAC_OUT1): ADC→DAC 光敏电压跟随 (软件触发)
 *              PA5 (DAC_OUT2): 波形发生器 (TIM6硬件触发, 噪声/三角波)
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB1 = 16MHz
 ****************************************************************************************************
 */

#include "dac.h"
#include "main.h"

/* ---- 三角波查找表 ---- */
static uint16_t g_tri_table[DAC_TRI_STEPS];

/* ---- 句柄 ---- */
static DAC_HandleTypeDef  DAC_Handler;     /* DAC 句柄 (CH1 + CH2 共用) */
static TIM_HandleTypeDef  TIM6_Handler;    /* TIM6 句柄 (波形发生器) */

/* ---- 波形发生器状态 ---- */
static volatile WaveType_t g_wave_type = WAVE_TRIANGLE;
static volatile uint16_t   g_dac_index = 0;
static uint16_t            g_noise_val  = 0;
static uint32_t            g_lfsr       = 0xABCD1234;

/* ================================================================
 * 三角波查表 / LFSR 噪声
 * ================================================================ */

static void DAC_GenTriTable(void)
{
    uint16_t i;
    uint16_t half = DAC_TRI_STEPS / 2;
    for (i = 0; i < half; i++)
        g_tri_table[i] = (uint16_t)(((uint32_t)i * DAC_TRI_MAX) / (half - 1));
    for (i = half; i < DAC_TRI_STEPS; i++)
        g_tri_table[i] = (uint16_t)(((uint32_t)(DAC_TRI_STEPS - 1 - i) * DAC_TRI_MAX) / (half - 1));
}

static uint16_t DAC_NoiseRand(void)
{
    uint16_t bit;
    uint8_t  i;
    for (i = 0; i < 4; i++)
    {
        bit = ((g_lfsr >> 0) ^ (g_lfsr >> 2) ^ (g_lfsr >> 3) ^ (g_lfsr >> 5)) & 1;
        g_lfsr = (g_lfsr >> 1) | (bit << 15);
    }
    return (uint16_t)(g_lfsr & 0x0FFF);
}

/* ================================================================
 * DAC 初始化 — 仅调用一次, 配置两个通道
 * 严格参考正点原子 dac_init() 的写法
 * ================================================================ */

/**
 * @brief       DAC 初始化 (PA4 CH1 + PA5 CH2)
 *
 *              CH1 (PA4): DAC_TRIGGER_NONE, 软件写 DHR→DOR 即刻更新
 *              CH2 (PA5): DAC_TRIGGER_T6_TRGO, 等待 TIM6 TRGO 锁存
 *
 * @param       无
 * @retval      无
 */
void DAC_Init(void)
{
    /* ---- 1. 使能时钟 ---- */
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* ---- 2. PA4 + PA5 模拟模式 ---- */
    GPIO_InitTypeDef gpio_init_struct = {0};
    gpio_init_struct.Mode = GPIO_MODE_ANALOG;
    gpio_init_struct.Pull = GPIO_NOPULL;

    gpio_init_struct.Pin = GPIO_PIN_4;           /* PA4 = DAC_OUT1 */
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    gpio_init_struct.Pin = GPIO_PIN_5;           /* PA5 = DAC_OUT2 */
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    /* ---- 3. 初始化 DAC 外设 ---- */
    DAC_Handler.Instance = DAC;
    HAL_DAC_Init(&DAC_Handler);

    /* ---- 4. 配置 CH1 (PA4): 无触发, 软件写入即更新 ---- */
    DAC_ChannelConfTypeDef dac_ch_conf = {0};
    dac_ch_conf.DAC_Trigger      = DAC_TRIGGER_NONE;           /* 软件触发 */
    dac_ch_conf.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    HAL_DAC_ConfigChannel(&DAC_Handler, &dac_ch_conf, DAC_CHANNEL_1);
    HAL_DAC_Start(&DAC_Handler, DAC_CHANNEL_1);

    /* ---- 5. 配置 CH2 (PA5): TIM6 TRGO 硬件触发 ---- */
    dac_ch_conf.DAC_Trigger = DAC_TRIGGER_T6_TRGO;             /* TIM6 TRGO */

    HAL_DAC_ConfigChannel(&DAC_Handler, &dac_ch_conf, DAC_CHANNEL_2);
    HAL_DAC_Start(&DAC_Handler, DAC_CHANNEL_2);
}

/* ================================================================
 * 电压设置 (参考 dac_set_voltage)
 * ================================================================ */

/**
 * @brief       设置 PA4 输出电压 (ADC→DAC 跟随)
 * @param       val: 12位 DAC 值 (0~4095)
 */
void DAC_SetCh1(uint16_t val)
{
    if (val > 4095U) val = 4095U;
    HAL_DAC_SetValue(&DAC_Handler, DAC_CHANNEL_1, DAC_ALIGN_12B_R, val);
}

/**
 * @brief       设置 PA5 输出电压 (波形发生器用)
 * @param       val: 12位 DAC 值 (0~4095)
 */
void DAC_SetCh2(uint16_t val)
{
    if (val > 4095U) val = 4095U;
    HAL_DAC_SetValue(&DAC_Handler, DAC_CHANNEL_2, DAC_ALIGN_12B_R, val);
}

/* ================================================================
 * 波形发生器 (TIM6)
 * 严格参考正点原子 Timer6_Init() 的写法
 * ================================================================ */

/**
 * @brief       启动 TIM6 波形发生器
 *
 *              TIM6 基础定时器:
 *              PSC = 15  → 16MHz/16 = 1MHz
 *              ARR = 39  → 1MHz/40  = 25kHz
 *
 *              TRGO = 更新事件 → 硬件触发 DAC CH2
 *              中断 → HAL_TIM_PeriodElapsedCallback → 更新 DAC CH2 值
 *
 * @param       无
 * @retval      无
 */
void DAC_WaveGen_Start(void)
{
    /* 预生成三角波表 */
    DAC_GenTriTable();

    /* ---- TIM6 基础配置 ---- */
    __HAL_RCC_TIM6_CLK_ENABLE();

    TIM6_Handler.Instance               = TIM6;
    TIM6_Handler.Init.Prescaler         = DAC_TIM6_PSC;           /* 15 */
    TIM6_Handler.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TIM6_Handler.Init.Period            = DAC_TIM6_ARR;           /* 39 */
    TIM6_Handler.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TIM6_Handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    HAL_TIM_Base_Init(&TIM6_Handler);

    /* 清除 HAL_TIM_Base_Init 产生的更新标志 */
    __HAL_TIM_CLEAR_FLAG(&TIM6_Handler, TIM_FLAG_UPDATE);

    /* 配置 TIM6 TRGO = 更新事件 */
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&TIM6_Handler, &sMasterConfig);

    /* 启动基础定时 + 更新中断 */
    HAL_TIM_Base_Start_IT(&TIM6_Handler);
}

/**
 * @brief       TIM6 底层 MSP 初始化 (HAL 自动调用)
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        __HAL_RCC_TIM6_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    }
}

/**
 * @brief       TIM6 周期溢出回调
 *
 *              TRGO 硬件触发 DAC 将上次写入 DHR 的值锁存到 DOR
 *              本回调写入下一个值到 DHR，等待下次 TRGO
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        switch (g_wave_type)
        {
            case WAVE_NOISE:
                g_noise_val = DAC_NoiseRand();
                DAC_SetCh2(g_noise_val);
                break;

            case WAVE_TRIANGLE:
                DAC_SetCh2(g_tri_table[g_dac_index]);
                g_dac_index++;
                if (g_dac_index >= DAC_TRI_STEPS)
                    g_dac_index = 0;
                break;

            default:
                break;
        }
    }
}

/* ================================================================
 * 波形切换
 * ================================================================ */

void DAC_WaveGen_SetType(WaveType_t type)
{
    g_wave_type = type;
    g_dac_index = 0;
    g_noise_val = 0;
}

WaveType_t DAC_WaveGen_GetType(void)
{
    return g_wave_type;
}

/* ================================================================
 * TIM6_DAC 共用中断向量
 * ================================================================ */

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TIM6_Handler);
}

/* ================================================================
 * DAC1 DMA 锯齿波 (TIM7 TRGO 触发, 128点)
 *
 * 硬件: PA4 (DAC_OUT1) → 示波器
 *       TIM7 TRGO → DAC1 外部触发 (1ms)
 *       DMA1 Stream5 CH7 → 循环搬运 128 个半字
 *
 * 锯齿波: 0 → 32 → 64 → ... → 4064 (128步, 步长=32)
 *         128 × 1ms = 128ms 一个完整周期
 * ================================================================ */

static DAC_HandleTypeDef hdac_saw;
static DMA_HandleTypeDef hdma_dac1;
uint32_t dac_saw_buf[DAC_SAW_STEPS];          /* uint32_t, DMA WORD 对齐 */

/**
 * @brief  生成 128 点锯齿波表 (12位值, 存于低12位)
 */
static void DAC_GenSawTable(void)
{
    uint32_t step = 4096U / DAC_SAW_STEPS;    /* 步长 = 32 */
    for (uint32_t i = 0; i < DAC_SAW_STEPS; i++)
    {
        dac_saw_buf[i] = i * step;            /* 0, 32, 64, ..., 4064 */
    }
}

/**
 * @brief  DAC1 DMA 初始化 (TIM7 TRGO, 锯齿波)
 */
void DAC_SAW_Init(void)
{
    /* ---- 1. 生成锯齿波数据 ---- */
    DAC_GenSawTable();

    /* ---- 2. GPIO: PA4 模拟输出 ---- */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* ---- 3. DMA1 Stream5 CH7 (DAC1) ---- */
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_dac1.Instance                 = DMA1_Stream5;
    hdma_dac1.Init.Channel             = DMA_CHANNEL_7;
    hdma_dac1.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_dac1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_dac1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_dac1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_dac1.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_dac1.Init.Mode                = DMA_CIRCULAR;
    hdma_dac1.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_dac1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_dac1) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(&hdac_saw, DMA_Handle1, hdma_dac1);

    /* ---- 4. DAC1 时钟 & 初始化 ---- */
    __HAL_RCC_DAC_CLK_ENABLE();

    hdac_saw.Instance = DAC;
    HAL_DAC_Init(&hdac_saw);

    /* ---- 5. CH1 (PA4): TIM7 TRGO 硬件触发 ---- */
    DAC_ChannelConfTypeDef ch = {0};
    ch.DAC_Trigger      = DAC_TRIGGER_T7_TRGO;
    ch.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    HAL_DAC_ConfigChannel(&hdac_saw, &ch, DAC_CHANNEL_1);
}

/**
 * @brief  启动 DAC1 DMA 锯齿波输出
 * @retval 0=成功, -1=失败
 */
int DAC_SAW_Start(void)
{
    if (HAL_DAC_Start_DMA(&hdac_saw, DAC_CHANNEL_1,
                          dac_saw_buf, DAC_SAW_STEPS,
                          DAC_ALIGN_12B_R) != HAL_OK)
    {
        return -1;
    }
    return 0;
}
