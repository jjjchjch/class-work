/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       ADC1 定时触发采样 + W25Q128 存储实验
 *
 *              硬件: STM32F407VET6 + ILI9341 2.8寸 LCD (FSMC, 240×320, RGB565)
 *                    W25Q128 Flash: CS=PC13, SCK=PA5, MISO=PA6, MOSI=PA7 (SPI1)
 *                    KEY1=PA0(下拉,按下高)
 *                    ADC1_IN2=PA2 (0~3V, 10Hz 正弦波输入)
 *                    UART1: PA9(TX), PA10(RX), 115200-8-N-1
 *
 *              功能:
 *              - TIM3 TRGO 每 10ms 触发 ADC1 转换 (PA2)
 *              - KEY1 启动采样, 采集 256 个结果存入 W25Q128 0x2000
 *              - LCD 简洁显示采样进度和结果
 *              - UART 同步打印采样数据
 *
 *              系统时钟: 16MHz HSI
 ****************************************************************************************************
 */

#include "main.h"
#include "key.h"
#include "delay.h"
#include "uart.h"
#include "timer.h"
#include "bsp_LCD_ILI9341.h"
#include "bsp_W25Q128.h"
#include <string.h>
#include <stdio.h>

/*===========================================================================
 * ADC 采样配置
 *===========================================================================*/
#define ADC_SAMPLE_COUNT    256             /* 采样点数 */
#define ADC_STORE_ADDR      0x00002000      /* W25Q128 存储地址 */

/* 采样缓冲区 (RAM) */
static volatile uint16_t g_adcBuf[ADC_SAMPLE_COUNT];
static volatile uint16_t g_adcIdx    = 0;   /* 当前采样索引 */
static volatile uint8_t  g_adcDone   = 0;   /* 采样完成标志 */
static volatile uint8_t  g_sampling  = 0;   /* 正在采样标志 */

/* ADC1 HAL 句柄 (供 adc.c 中断使用) */
ADC_HandleTypeDef hadc1 = {0};

/*===========================================================================
 * 串口打印辅助
 *===========================================================================*/

static void UART_Println(const char *msg)
{
    UART1_SendString(msg);
    UART1_SendString("\r\n");
}

static void UART_PrintSep(void)
{
    UART1_SendString("----------------------------------------\r\n");
}

/*===========================================================================
 * LCD 简洁界面
 *===========================================================================*/

static void LCD_DrawUI(void)
{
    LCD_Fill(0, 0, LCD_WIDTH - 1, LED_HEIGHT - 1, BLACK);

    /* 标题 */
    LCD_Fill(0, 0, LCD_WIDTH - 1, 32, 0x2104);
    LCD_String(30, 5, (char *)"ADC Sampling Test", 16, WHITE, 0x2104);
    LCD_Fill(0, 32, LCD_WIDTH - 1, 34, GREEN);

    /* 信息 */
    LCD_String(5, 42, (char *)"ADC1: PA2  0~3V  10Hz Sine", 12, CYAN, BLACK);
    LCD_String(5, 60, (char *)"Trigger: TIM3 TRGO  10ms", 12, CYAN, BLACK);
    LCD_String(5, 78, (char *)"Store: W25Q128 @ 0x2000", 12, CYAN, BLACK);
    LCD_Fill(0, 98, LCD_WIDTH - 1, 100, GREEN);

    /* 操作提示 */
    LCD_String(5, 108, (char *)"[KEY1] Start Sampling 256pts", 16, WHITE, BLACK);
    LCD_Fill(0, 136, LCD_WIDTH - 1, 138, GREEN);

    /* 状态区标签 */
    LCD_String(5, 146, (char *)"Status:", 12, YELLOW, BLACK);
}

static void LCD_ShowStatus(const char *msg, uint16_t color)
{
    LCD_Fill(5, 168, LCD_WIDTH - 6, 250, BLACK);
    LCD_String(5, 170, (char *)msg, 16, color, BLACK);
}

static void LCD_ShowProgress(uint16_t n)
{
    char buf[32];
    LCD_Fill(5, 168, LCD_WIDTH - 6, 210, BLACK);
    sprintf(buf, "Sampling... %u / %u", n, ADC_SAMPLE_COUNT);
    LCD_String(5, 172, buf, 16, YELLOW, BLACK);
}

/*===========================================================================
 * LCD 波形绘制 (显示前 N 个采样点, 约 6 个正弦周期)
 *===========================================================================*/

#define WAV_X0      10
#define WAV_Y0      135
#define WAV_X1      230
#define WAV_Y1      290
#define WAV_W       (WAV_X1 - WAV_X0)   /* 220 像素 */
#define WAV_H       (WAV_Y1 - WAV_Y0)   /* 155 像素 */
#define WAV_DISP_CNT 60                  /* 显示前60点 (6个周期, 10Hz/100Hz) */

static void LCD_DrawWaveform(void)
{
    char buf[32];
    uint16_t i;
    uint16_t cnt = (ADC_SAMPLE_COUNT < WAV_DISP_CNT) ? ADC_SAMPLE_COUNT : WAV_DISP_CNT;

    /* ---- 清除波形区域 ---- */
    LCD_Fill(0, 132, LCD_WIDTH - 1, LED_HEIGHT - 1, BLACK);

    /* ---- 标题 ---- */
    LCD_String(5, 137, (char *)"ADC Waveform (PA2) 10Hz Sine", 12, CYAN, BLACK);

    /* ---- 外框 ---- */
    LCD_Fill(WAV_X0 - 1, WAV_Y0 - 1, WAV_X1 + 1, WAV_Y1 + 1, 0x18E3);
    LCD_Fill(WAV_X0, WAV_Y0, WAV_X1, WAV_Y1, BLACK);

    /* ---- 水平网格: 3.0V, 1.5V, 0V ---- */
    uint16_t midY = WAV_Y0 + WAV_H / 2;
    LCD_Line(WAV_X0, WAV_Y0, WAV_X1, WAV_Y0, 0x39E7);  /* 顶部 3.3V */
    LCD_Line(WAV_X0, midY,   WAV_X1, midY,   0x3186);  /* 中间 1.65V */
    LCD_Line(WAV_X0, WAV_Y1, WAV_X1, WAV_Y1, 0x39E7);  /* 底部 0V */

    /* ---- Y 轴标签 ---- */
    LCD_String(WAV_X1 + 4, WAV_Y0 - 4, (char *)"3.3V", 12, 0x8410, BLACK);
    LCD_String(WAV_X1 + 4, midY   - 4, (char *)"1.6V", 12, 0x8410, BLACK);
    LCD_String(WAV_X1 + 4, WAV_Y1 - 6, (char *)"0.0V", 12, 0x8410, BLACK);

    /* ---- 绘制波形 (连线, 只画前 cnt 个点) ---- */
    for (i = 1; i < cnt; i++)
    {
        uint16_t x0 = WAV_X0 + (uint16_t)((uint32_t)(i - 1) * WAV_W / (cnt - 1));
        uint16_t x1 = WAV_X0 + (uint16_t)((uint32_t)i       * WAV_W / (cnt - 1));

        uint16_t y0 = WAV_Y1 - (uint16_t)((uint32_t)g_adcBuf[i - 1] * WAV_H / 4095);
        uint16_t y1 = WAV_Y1 - (uint16_t)((uint32_t)g_adcBuf[i]     * WAV_H / 4095);

        if (y0 < WAV_Y0) y0 = WAV_Y0;
        if (y1 < WAV_Y0) y1 = WAV_Y0;
        if (y0 > WAV_Y1) y0 = WAV_Y1;
        if (y1 > WAV_Y1) y1 = WAV_Y1;

        LCD_Line(x0, y0, x1, y1, YELLOW);
    }

    /* ---- Vpp 信息 ---- */
    uint16_t vmin = 4095, vmax = 0;
    for (i = 0; i < ADC_SAMPLE_COUNT; i++)
    {
        if (g_adcBuf[i] < vmin) vmin = g_adcBuf[i];
        if (g_adcBuf[i] > vmax) vmax = g_adcBuf[i];
    }
    sprintf(buf, "Vpp=%.2fV  %u pts", (float)(vmax - vmin) * 3.3f / 4096.0f, ADC_SAMPLE_COUNT);
    LCD_String(5, 298, buf, 12, 0xFFE0, BLACK);
    LCD_String(140, 298, (char *)"Saved W25Q128", 12, 0x8410, BLACK);
}

/*===========================================================================
 * ADC1 定时触发采样 (TIM3 TRGO, 10ms)
 *===========================================================================*/

/**
 * @brief  ADC1 PA2 初始化 (TIM3 TRGO 外部触发, 中断模式)
 *
 *         PA2 → ADC1_IN2, 模拟输入
 *         触发源: TIM3 TRGO (每 10ms 触发一次)
 *         EOC 中断 → adc.c HAL_ADC_ConvCpltCallback → ADC1_ConvCpltCallback
 */
static void ADC1_PA2_Init(void)
{
    /* ---- GPIO: PA2 模拟输入 ---- */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* ---- ADC1 配置 ---- */
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;   /* 4MHz */
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.NbrOfDiscConversion   = 0;
    hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T3_TRGO;    /* TIM3 TRGO */
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.DMAContinuousRequests = DISABLE;

    HAL_ADC_Init(&hadc1);

    /* ---- 通道 CH2 (PA2) ---- */
    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_2;
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    ch.Offset       = 0;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    /* ---- ADC 中断使能 ---- */
    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
}

/**
 * @brief  启动 ADC 采样 (使能 TIM3 + ADC1 中断)
 */
static void ADC1_StartSampling(void)
{
    g_adcIdx   = 0;
    g_adcDone  = 0;
    g_sampling = 1;
    memset((void *)g_adcBuf, 0, sizeof(g_adcBuf));

    /* 启动 ADC1 中断模式 (等待 TIM3 TRGO) */
    HAL_ADC_Start_IT(&hadc1);

    /* 启动 TIM3 (TRGO 输出) */
    TIM3_ADC_Trigger_Init();
}

/**
 * @brief  停止 ADC 采样
 */
static void ADC1_StopSampling(void)
{
    HAL_ADC_Stop_IT(&hadc1);
    TIM3->CR1 &= ~TIM_CR1_CEN;      /* 停止 TIM3 */
    g_sampling = 0;
}

/**
 * @brief  ADC1 转换完成回调 (由 adc.c HAL_ADC_ConvCpltCallback 调用)
 *         每次 TIM3 TRGO 触发 → ADC 转换完成 → 进入此函数
 */
void ADC1_ConvCpltCallback(uint16_t val)
{
    if (!g_sampling) return;

    g_adcBuf[g_adcIdx++] = val;

    if (g_adcIdx >= ADC_SAMPLE_COUNT)
    {
        g_adcDone = 1;
        ADC1_StopSampling();
    }
}

/*===========================================================================
 * W25Q128 存储 ADC 数据
 *===========================================================================*/

/**
 * @brief  将 256 个 ADC 采样值保存到 W25Q128
 * @retval 0=成功, 1=失败
 */
static uint8_t SaveToW25Q128(void)
{
    if (xW25Q128.FlagInit == 0) return 1;

    UART_Println("Writing ADC samples to W25Q128...");

    /* 256 个 uint16_t = 512 字节 */
    W25Q128_WriteData(ADC_STORE_ADDR, (uint8_t *)g_adcBuf,
                      ADC_SAMPLE_COUNT * sizeof(uint16_t));

    /* 回读验证前几个值 */
    uint16_t verify[4];
    W25Q128_ReadData(ADC_STORE_ADDR, (uint8_t *)verify, sizeof(verify));

    UART_Println("Verify first 4 samples:");
    for (int i = 0; i < 4; i++)
    {
        char buf[48];
        sprintf(buf, "  [%d] Wrote=%u  Read=%u  %s",
                i, g_adcBuf[i], verify[i],
                (g_adcBuf[i] == verify[i]) ? "OK" : "FAIL");
        UART_Println(buf);
    }

    if (memcmp((const void *)g_adcBuf, verify, sizeof(verify)) == 0)
    {
        UART_Println("Save to W25Q128 SUCCESS!");
        return 0;
    }
    UART_Println("Save to W25Q128 FAILED!");
    return 1;
}

/*===========================================================================
 * 主函数
 *===========================================================================*/

static void SystemClock_Config(void);

int main(void)
{
    uint8_t keyVal;

    /* ---- 硬件初始化 ---- */
    HAL_Init();
    SystemClock_Config();

    /* ---- UART1 ---- */
    UART1_Init();
    UART_PrintSep();
    UART_Println("=== ADC1 Timer-Triggered Sampling ===");
    UART_Println("ADC1: PA2 (IN2)  0~3V  10Hz Sine");
    UART_Println("TIM3: TRGO 10ms  -> 256 samples");
    UART_Println("Store: W25Q128 @ 0x2000");
    UART_PrintSep();

    /* ---- LCD ---- */
    LCD_Init();
    LCD_DrawUI();
    UART_Println("LCD Init OK");

    /* ---- 按键 ---- */
    key_init();
    UART_Println("KEY1=PA0  (Start Sampling)");

    /* ---- W25Q128 ---- */
    uint8_t w25q_ok = W25Q128_Init();
    if (w25q_ok)
    {
        char buf[40];
        sprintf(buf, "W25Q128 OK [%s]", xW25Q128.type);
        UART_Println(buf);
        LCD_String(5, 170, buf, 12, GREEN, BLACK);
    }
    else
    {
        UART_Println("W25Q128 FAIL!");
        LCD_String(5, 170, (char *)"W25Q128 FAIL!", 12, RED, BLACK);
    }

    /* ---- ADC1 初始化 ---- */
    ADC1_PA2_Init();
    UART_Println("ADC1 Init OK (PA2)");
    UART_PrintSep();

    /* ---- 主循环 ---- */
    while (1)
    {
        keyVal = key_scan(0);

        if (keyVal == KEY1_PRES && !g_sampling)
        {
            /* ====== KEY1: 启动采样 ====== */
            UART_PrintSep();
            UART_Println(">>> KEY1: Start Sampling 256 pts <<<");

            ADC1_StartSampling();
            LCD_ShowStatus("Sampling started...", YELLOW);

            /* 等待采样完成 (ADC 中断自动采集) */
            uint16_t lastIdx = 0;
            while (!g_adcDone)
            {
                if (g_adcIdx != lastIdx)
                {
                    lastIdx = g_adcIdx;
                    LCD_ShowProgress(lastIdx);
                }
                HAL_Delay(5);
            }

            /* ---- LCD 绘制波形 (常驻, 不跳出) ---- */
            LCD_DrawWaveform();

            /* 保存到 W25Q128 (后台进行, 不覆盖波形) */
            uint8_t saveOk = SaveToW25Q128();

            if (saveOk == 0)
            {
                UART_Println("[OK] Saved to W25Q128 @ 0x2000");
            }
            else
            {
                UART_Println("[FAIL] W25Q128 write error!");
            }
            UART_PrintSep();
        }

        HAL_Delay(20);
    }
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif

