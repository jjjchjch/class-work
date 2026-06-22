/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       DAC三角波发生 + ADC定时采样 + DSP统计分析 + LCD波形显示
 *
 *              硬件: STM32F407VET6 + ILI9341 2.8寸 LCD (FSMC, 240×320, RGB565)
 *                    PA4 (DAC_OUT1) → 跳线 → PA2 (ADC1_IN2)
 *                    UART1: PA9(TX), PA10(RX), 115200bps
 *
 *              功能:
 *              - PA4 产生 0~3V 100Hz 三角波 (TIM6触发DAC, DMA循环)
 *              - TIM2 100μs 定时触发 ADC1 采样 PA2 (256点/批)
 *              - 采集完256点自动停止, LCD 绘制波形
 *              - 用 arm_max_f32 / arm_min_f32 / arm_mean_f32 / arm_rms_f32 计算统计量
 *              - LCD 显示 Vmax / Vmin / Vavg / Vrms
 *              - 自动循环: 显示 → 2s延时 → 重新采集
 *
 *              系统时钟: 16MHz HSI
 *
 *              【接线说明】用杜邦线将 PA4 与 PA2 短接
 ****************************************************************************************************
 */

#include "main.h"
#include "key.h"
#include "uart.h"
#include "bsp_LCD_ILI9341.h"
#include "arm_math.h"
#include <stdio.h>
#include <string.h>

/*===========================================================================
 * 常量定义
 *===========================================================================*/
#define TRI_POINTS      256U    /* 三角波表点数                    */
#define ADC_COUNT       256U    /* 每批 ADC 采样点数               */
#define DAC_VMAX        3723U   /* 3.0V / 3.3V * 4095 ≈ 3723      */
#define DAC_VREF        3.3f    /* DAC 参考电压                     */

/* TIM6: 16MHz / (PSC+1) / (ARR+1) = 16M / 1 / 625 = 25600Hz      */
/* 三角波频率 = 25600 / 256 = 100Hz                                */
#define TIM6_PSC        0U
#define TIM6_ARR        624U

/* TIM2: 16MHz / (PSC+1) / (ARR+1) = 16M / 16 / 100 = 10kHz       */
/* ADC 采样间隔 = 100μs                                            */
#define TIM2_PSC        15U
#define TIM2_ARR        99U

/* ---- LCD 颜色 ---- */
#define BLACK   0x0000
#define WHITE   0xFFFF
#define YELLOW  0xFFE0
#define CYAN    0x7FFF
#define GREEN   0x07E0
#define RED     0xF800
#define BLUE2   0x051F

/*===========================================================================
 * 三角波查找表 (RAM, DMA 循环使用)
 *===========================================================================*/
static uint16_t g_triTableRAM[TRI_POINTS];

/*===========================================================================
 * ADC 采样缓冲区
 *===========================================================================*/
static volatile uint16_t g_adcBuf[ADC_COUNT];
static volatile uint16_t g_adcIdx  = 0;
static volatile uint8_t  g_adcDone = 0;

/*===========================================================================
 * 外设句柄
 *===========================================================================*/
static DAC_HandleTypeDef  hdac;
static TIM_HandleTypeDef  htim6;   /* DAC 触发定时器 */
static TIM_HandleTypeDef  htim2;   /* ADC 触发定时器 */
static DMA_HandleTypeDef  hdma_dac1;

/* hadc1 给 adc.c 的 ISR 使用 (必须保留) */
ADC_HandleTypeDef hadc1 = {0};

/*===========================================================================
 * 三角波表生成 (运行时, 填充到 RAM 数组)
 * 波形: 0 → VMAX → 0, 无负值, 256点
 *===========================================================================*/
static void GenTriTable(uint16_t *table, uint16_t points, uint16_t vmax)
{
    uint16_t i;
    uint16_t half = points / 2U;  /* 128 */

    for (i = 0; i < half; i++)
    {
        table[i] = (uint16_t)(((uint32_t)i * vmax) / (half - 1U));
    }
    for (i = half; i < points; i++)
    {
        table[i] = (uint16_t)(((uint32_t)(points - 1U - i) * vmax) / (half - 1U));
    }
}

/*===========================================================================
 * DAC 三角波初始化 (PA4, TIM6 触发, DMA1_Stream5 循环)
 *===========================================================================*/
static void DAC_Tri_Init(void)
{
    /* ---- 1. 生成三角波表到 RAM ---- */
    GenTriTable(g_triTableRAM, TRI_POINTS, DAC_VMAX);

    /* ---- 2. GPIO: PA4 模拟模式 ---- */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* ---- 3. DAC 初始化 ---- */
    __HAL_RCC_DAC_CLK_ENABLE();
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);

    /* ---- 4. DAC CH1 (PA4): TIM6 TRGO 硬件触发 ---- */
    DAC_ChannelConfTypeDef dac_ch = {0};
    dac_ch.DAC_Trigger      = DAC_TRIGGER_T6_TRGO;
    dac_ch.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &dac_ch, DAC_CHANNEL_1);

    /* ---- 5. DMA1 Stream5 Channel7: 内存→DAC, 循环模式 ---- */
    __HAL_RCC_DMA1_CLK_ENABLE();
    hdma_dac1.Instance                 = DMA1_Stream5;
    hdma_dac1.Init.Channel             = DMA_CHANNEL_7;
    hdma_dac1.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_dac1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_dac1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_dac1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_dac1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_dac1.Init.Mode                = DMA_CIRCULAR;
    hdma_dac1.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_dac1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_dac1);
    __HAL_LINKDMA(&hdac, DMA_Handle1, hdma_dac1);

    /* ---- 6. 启动 DAC DMA ---- */
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1,
                      (uint32_t *)g_triTableRAM, TRI_POINTS,
                      DAC_ALIGN_12B_R);

    /* ---- 7. TIM6: 触发 DAC 的定时器 (25600Hz) ---- */
    __HAL_RCC_TIM6_CLK_ENABLE();
    htim6.Instance               = TIM6;
    htim6.Init.Prescaler         = TIM6_PSC;    /* 0 */
    htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim6.Init.Period            = TIM6_ARR;    /* 624 → 25600Hz */
    htim6.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim6);

    /* TRGO = Update event → 触发 DAC */
    TIM_MasterConfigTypeDef master = {0};
    master.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim6, &master);

    /* 仅启动基础定时 (无需中断, DMA 自动搬运) */
    HAL_TIM_Base_Start(&htim6);
}

/*===========================================================================
 * ADC 初始化 (PA2, TIM2 TRGO 触发, 100μs)
 *===========================================================================*/
static void ADC_Sample_Init(void)
{
    /* ---- 1. GPIO: PA2 模拟输入 ---- */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* ---- 2. ADC1 基础配置 ---- */
    __HAL_RCC_ADC1_CLK_ENABLE();
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;  /* 4MHz */
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.NbrOfDiscConversion   = 0;
    hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T2_TRGO;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    HAL_ADC_Init(&hadc1);

    /* ---- 3. ADC 通道: CH2 (PA2) ---- */
    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_2;
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_144CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    /* ---- 4. ADC 中断 ---- */
    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);

    /* ---- 5. TIM2: ADC 触发定时器 (100μs) ---- */
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = TIM2_PSC;    /* 15  → 1MHz */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = TIM2_ARR;    /* 99  → 100μs */
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim2);

    /* TRGO = Update event → 触发 ADC */
    TIM_MasterConfigTypeDef master = {0};
    master.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &master);
}

/*===========================================================================
 * ADC 采样控制
 *===========================================================================*/
static void ADC_Start(void)
{
    g_adcIdx  = 0;
    g_adcDone = 0;
    memset((void *)g_adcBuf, 0, sizeof(g_adcBuf));

    HAL_TIM_Base_Start(&htim2);           /* 启动 TIM2 触发 */
    HAL_ADC_Start_IT(&hadc1);             /* 启动 ADC 中断采样 */
}

static void ADC_Stop(void)
{
    HAL_ADC_Stop_IT(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
}

/*===========================================================================
 * 串口发送波形数据 (纯ADC值, 每行一个, 供 SerialPlot 显示)
 * 格式: ADC值\n   (仅换行符, 无回车)
 * SerialPlot: Baud=115200, ASCII, uint16, 分隔符=Newline, 通道数=1
 *===========================================================================*/
static void SendToSerialPlot(void)
{
    uint16_t i;
    char buf[12];

    for (i = 0; i < ADC_COUNT; i++)
    {
        int len = sprintf(buf, "%u\n", g_adcBuf[i]);
        UART1_SendString(buf);
        (void)len;
    }
}

/*===========================================================================
 * ADC 转换完成回调
 * (由 adc.c 的 ADC_IRQHandler → HAL_ADC_IRQHandler 调用)
 *===========================================================================*/
void ADC1_ConvCpltCallback(uint16_t val)
{
    if (g_adcDone) return;

    g_adcBuf[g_adcIdx++] = val;

    if (g_adcIdx >= ADC_COUNT)
    {
        g_adcDone = 1;
        ADC_Stop();
    }
}

/*===========================================================================
 * LCD 波形绘制
 *===========================================================================*/
#define WAV_X0   5
#define WAV_Y0   100
#define WAV_X1   235
#define WAV_Y1   310
#define WAV_W    (WAV_X1 - WAV_X0)
#define WAV_H    (WAV_Y1 - WAV_Y0)

static void LCD_DrawWaveform(void)
{
    uint16_t i;

    /* 清波形区域 */
    LCD_Fill(0, WAV_Y0 - 2, LCD_WIDTH - 1, LED_HEIGHT - 1, BLACK);

    /* 波形边框 */
    LCD_Fill(WAV_X0 - 1, WAV_Y0 - 1, WAV_X1 + 1, WAV_Y1 + 1, 0x18E3);
    LCD_Fill(WAV_X0, WAV_Y0, WAV_X1, WAV_Y1, BLACK);

    /* 参考线: 3.0V(顶), 1.5V(中), 0V(底) */
    uint16_t yTop = WAV_Y0;
    uint16_t yMid = WAV_Y0 + WAV_H / 2;
    uint16_t yBot = WAV_Y1;

    LCD_Line(WAV_X0, yTop, WAV_X1, yTop, 0x39E7);
    LCD_Line(WAV_X0, yMid, WAV_X1, yMid, 0x3186);
    LCD_Line(WAV_X0, yBot, WAV_X1, yBot, 0x39E7);

    /* 电压标注 */
    LCD_String(WAV_X1 + 4, yTop - 4, (char *)"3.0V", 12, 0x8410, BLACK);
    LCD_String(WAV_X1 + 4, yMid - 4, (char *)"1.5V", 12, 0x8410, BLACK);
    LCD_String(WAV_X1 + 4, yBot - 6, (char *)"0.0V", 12, 0x8410, BLACK);

    /* 绘制波形 (256点折线) */
    for (i = 1; i < ADC_COUNT; i++)
    {
        uint16_t x0 = WAV_X0 + (uint32_t)(i - 1) * WAV_W / (ADC_COUNT - 1);
        uint16_t x1 = WAV_X0 + (uint32_t)i       * WAV_W / (ADC_COUNT - 1);

        /* ADC值 → Y坐标 (0V=底部) */
        uint16_t y0 = WAV_Y1 - (uint32_t)g_adcBuf[i - 1] * WAV_H / 4095U;
        uint16_t y1 = WAV_Y1 - (uint32_t)g_adcBuf[i]     * WAV_H / 4095U;

        if (y0 < WAV_Y0) y0 = WAV_Y0;
        if (y1 < WAV_Y0) y1 = WAV_Y0;
        if (y0 > WAV_Y1) y0 = WAV_Y1;
        if (y1 > WAV_Y1) y1 = WAV_Y1;

        LCD_Line(x0, y0, x1, y1, YELLOW);
    }
}

/*===========================================================================
 * LCD 统计数据显示 + DSP 库统计计算
 *===========================================================================*/
static void LCD_ShowStats(void)
{
    uint16_t i;
    char buf[48];

    /* 将 ADC 值转为 float 电压数组 (供 DSP 库使用) */
    float fBuf[ADC_COUNT];
    for (i = 0; i < ADC_COUNT; i++)
    {
        fBuf[i] = (float)g_adcBuf[i] * DAC_VREF / 4095.0f;
    }

    /* ---- CMSIS-DSP 统计计算 ---- */
    float fMax, fMin, fMean, fRms;
    uint32_t index;
    arm_max_f32(fBuf, ADC_COUNT, &fMax, &index);
    arm_min_f32(fBuf, ADC_COUNT, &fMin, &index);
    arm_mean_f32(fBuf, ADC_COUNT, &fMean);
    arm_rms_f32(fBuf, ADC_COUNT, &fRms);

    /* 原始 ADC 值统计 */
    uint16_t adcMax = 0, adcMin = 4095;
    for (i = 0; i < ADC_COUNT; i++)
    {
        if (g_adcBuf[i] > adcMax) adcMax = g_adcBuf[i];
        if (g_adcBuf[i] < adcMin) adcMin = g_adcBuf[i];
    }

    /* ---- LCD 顶部状态栏 ---- */
    LCD_Fill(0, 0, LCD_WIDTH - 1, 28, 0x2104);
    LCD_String(8, 4, (char *)"DAC Tri 100Hz + ADC 100us", 16, WHITE, 0x2104);
    LCD_Fill(0, 28, LCD_WIDTH - 1, 30, GREEN);

    /* ---- 电压统计 (DSP库) ---- */
    LCD_String(5, 34, (char *)"-- Voltage Stats (DSP Lib) --", 12, YELLOW, BLACK);

    sprintf(buf, "Vmax:  %5.3f V", (double)fMax);
    LCD_String(8, 50, buf, 12, WHITE, BLACK);

    sprintf(buf, "Vmin:  %5.3f V", (double)fMin);
    LCD_String(8, 64, buf, 12, WHITE, BLACK);

    sprintf(buf, "Vavg:  %5.3f V", (double)fMean);
    LCD_String(8, 78, buf, 12, CYAN, BLACK);

    sprintf(buf, "Vrms:  %5.3f V", (double)fRms);
    LCD_String(8, 92, buf, 12, GREEN, BLACK);

    /* ---- ADC 原始值 ---- */
    sprintf(buf, "ADCmax=%u  ADCmin=%u", adcMax, adcMin);
    LCD_String(8, 108, buf, 12, 0x8410, BLACK);
}

/*===========================================================================
 * 系统时钟: HSI 16MHz
 *===========================================================================*/
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
        Error_Handler();
}

/*===========================================================================
 * 主函数
 *===========================================================================*/
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    /* ---- UART ---- */
    UART1_Init();

    /* ---- LCD ---- */
    LCD_Init();

    /* ---- Keys ---- */
    key_init();

    /* ---- 启动 DAC 三角波 (PA4, 后台持续运行) ---- */
    DAC_Tri_Init();

    /* ---- 初始化 ADC 采样 (PA2) ---- */
    ADC_Sample_Init();

    /* ---- 主循环: 采集→显示→等待→重复 ---- */
    uint32_t batchNum = 0;
    for (;;)
    {
        batchNum++;

        /* 开始采集 256 点 */
        ADC_Start();

        /* 等待采集完成 */
        uint16_t lastIdx = 0;
        while (!g_adcDone)
        {
            if (g_adcIdx != lastIdx)
            {
                lastIdx = g_adcIdx;
                char buf[24];
                sprintf(buf, "Sampling %3u / %u", lastIdx, ADC_COUNT);
                LCD_Fill(100, 164, 230, 178, BLACK);
                LCD_String(100, 164, buf, 12, YELLOW, BLACK);
            }
            HAL_Delay(1);
        }

        /* 绘制波形 */
        LCD_DrawWaveform();

        /* 发送纯 ADC 数据到串口示波器 */
        SendToSerialPlot();

        /* 计算并显示统计量 */
        LCD_ShowStats();

        /* 批次提示 */
        {
            char buf[32];
            sprintf(buf, "Batch#%lu  KEY1:ReSample", (unsigned long)batchNum);
            LCD_String(8, 152, buf, 12, CYAN, BLACK);
        }

        /* 延时 2s 或按键触发重采 */
        uint32_t wait = 0;
        while (wait < 200)
        {
            if (key_scan(0) == KEY1_PRES) break;
            HAL_Delay(10);
            wait++;
        }
    }
}

void Error_Handler(void) { __disable_irq(); while (1) {} }

