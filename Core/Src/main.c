/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       月薪喵跳舞 + ADC 采样 (原始 22 帧 128×64 数据移植)
 *
 *              硬件: STM32F407VET6 + ILI9341 2.8寸 LCD (FSMC, 240×320, RGB565)
 *                    ADC1_IN2=PA2, KEY1=PA0, KEY2=PA1
 *                    UART1: PA9(TX), PA10(RX), 115200bps
 *
 *              功能:
 *              - KEY2 切换: 月薪喵动画模式 (22帧循环, 原始CSDN数据)
 *              - KEY1 启动: ADC 采样 256 点 + 波形显示
 *
 *              帧数据: Core/Inc/oled_frames.h (22×1024 bytes, Flash)
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
#include "oled_frames.h"
#include "dsp_test.h"
#include <string.h>
#include <stdio.h>

/*===========================================================================
 * ADC 采样配置
 *===========================================================================*/
#define ADC_SAMPLE_COUNT    256
#define ADC_STORE_ADDR      0x00002000

static volatile uint16_t g_adcBuf[ADC_SAMPLE_COUNT];
static volatile uint16_t g_adcIdx   = 0;
static volatile uint8_t  g_adcDone  = 0;
static volatile uint8_t  g_sampling = 0;

ADC_HandleTypeDef hadc1 = {0};

/*===========================================================================
 * 显示模式
 *===========================================================================*/
static uint8_t g_showMoonCat = 0;
static uint8_t g_mcatFrame   = 0;
static uint8_t g_dspMode     = 0;  /* 0=正常, 1=DSP测试模式 */

/*===========================================================================
 * 串口辅助
 *===========================================================================*/
static void UART_Println(const char *msg) {
    UART1_SendString(msg);
    UART1_SendString("\r\n");
}
static void UART_PrintSep(void) {
    UART1_SendString("----------------------------------------\r\n");
}

/*===========================================================================
 * 月薪喵绘制 (LCD_Fill 直接画, 不擦不叠, 逐帧覆盖)
 *===========================================================================*/
#define MCAT_SCALE    2
#define MCAT_DISP_W   (MOONCAT_W * MCAT_SCALE)    /* 256 */
#define MCAT_DISP_H   (MOONCAT_H * MCAT_SCALE)    /* 128 */
#define MCAT_X0       ((LCD_WIDTH  - MCAT_DISP_W) / 2)  /* -8 */
#define MCAT_Y0       ((LED_HEIGHT - MCAT_DISP_H) / 2)  /* 96  */

static void MoonCat_DrawFrame(uint8_t frameIdx)
{
    if (frameIdx >= MOONCAT_FRAMES) return;

    const uint8_t (*frame)[MOONCAT_W] = YueXinMao[frameIdx];

    for (uint8_t page = 0; page < 8; page++)
    {
        for (uint8_t col = 0; col < MOONCAT_W; col++)
        {
            uint8_t byteVal = frame[page][col];
            if (byteVal == 0) continue;

            int16_t baseX = MCAT_X0 + (int16_t)(col * MCAT_SCALE);
            int16_t baseY = MCAT_Y0 + (int16_t)(page * 8 * MCAT_SCALE);

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                if (!(byteVal & (1 << bit))) continue;

                int16_t px = baseX;
                int16_t py = baseY + (int16_t)(bit * MCAT_SCALE);
                int16_t w  = MCAT_SCALE;
                int16_t h  = MCAT_SCALE;

                /* 裁剪 */
                if (px < 0) { w += px; px = 0; }
                if (py < 0) { h += py; py = 0; }
                if (px + w > LCD_WIDTH)  w = LCD_WIDTH - px;
                if (py + h > LED_HEIGHT) h = LED_HEIGHT - py;
                if (w <= 0 || h <= 0) continue;

                LCD_Fill((uint16_t)px, (uint16_t)py,
                         (uint16_t)(px + w - 1), (uint16_t)(py + h - 1), 0xFFFF);
            }
        }
    }
}

/*===========================================================================
 * LCD UI
 *===========================================================================*/
static void LCD_DrawADCUI(void)
{
    LCD_Fill(0, 0, LCD_WIDTH - 1, LED_HEIGHT - 1, BLACK);
    LCD_Fill(0, 0, LCD_WIDTH - 1, 32, 0x2104);
    LCD_String(15, 5, (char *)"ADC + MoonCat + DSP", 16, WHITE, 0x2104);
    LCD_Fill(0, 32, LCD_WIDTH - 1, 34, GREEN);
    LCD_String(5, 42, (char *)"KEY1:ADC Sample  256pts", 12, CYAN, BLACK);
    LCD_String(5, 58, (char *)"KEY2:YueXinMiao Dance", 12, CYAN, BLACK);
    LCD_String(5, 74, (char *)"KEY3:DSP Sine/Cos Test", 12, CYAN, BLACK);
}

static void LCD_DrawMoonCatUI(void)
{
    LCD_Fill(0, 0, LCD_WIDTH - 1, LED_HEIGHT - 1, BLACK);
    LCD_Fill(0, 0, LCD_WIDTH - 1, 32, 0x2104);
    LCD_String(18, 5, (char *)"YueXinMiao Dance!", 16, WHITE, 0x2104);
    LCD_Fill(0, 32, LCD_WIDTH - 1, 34, GREEN);
    LCD_String(5, 42, (char *)"KEY1:ADC  KEY2:Exit", 12, CYAN, BLACK);
    LCD_String(5, 58, (char *)"22frames 128x64@2x 20FPS", 12, 0x8410, BLACK);
}

/* ---- 波形绘制 ---- */
#define WAV_X0      5
#define WAV_Y0      80
#define WAV_X1      235
#define WAV_Y1      230
#define WAV_W       (WAV_X1 - WAV_X0)
#define WAV_H       (WAV_Y1 - WAV_Y0)

static void LCD_DrawWaveform(void)
{
    char buf[32];
    uint16_t i;
    uint16_t cnt = 60;  /* 显示前60点 (6个周期) */

    LCD_Fill(0, 70, LCD_WIDTH - 1, LED_HEIGHT - 1, BLACK);

    LCD_Fill(WAV_X0-1, WAV_Y0-1, WAV_X1+1, WAV_Y1+1, 0x18E3);
    LCD_Fill(WAV_X0, WAV_Y0, WAV_X1, WAV_Y1, BLACK);

    uint16_t midY = WAV_Y0 + WAV_H/2;
    LCD_Line(WAV_X0, WAV_Y0, WAV_X1, WAV_Y0, 0x39E7);
    LCD_Line(WAV_X0, midY,   WAV_X1, midY,   0x3186);
    LCD_Line(WAV_X0, WAV_Y1, WAV_X1, WAV_Y1, 0x39E7);

    LCD_String(WAV_X1+4, WAV_Y0-4, (char *)"3.3V", 12, 0x8410, BLACK);
    LCD_String(WAV_X1+4, midY-4,   (char *)"1.6V", 12, 0x8410, BLACK);
    LCD_String(WAV_X1+4, WAV_Y1-6, (char *)"0.0V", 12, 0x8410, BLACK);

    for (i = 1; i < cnt; i++)
    {
        uint16_t x0 = WAV_X0 + (uint32_t)(i-1) * WAV_W / (cnt-1);
        uint16_t x1 = WAV_X0 + (uint32_t)i     * WAV_W / (cnt-1);
        uint16_t y0 = WAV_Y1 - (uint32_t)g_adcBuf[i-1] * WAV_H / 4095;
        uint16_t y1 = WAV_Y1 - (uint32_t)g_adcBuf[i]   * WAV_H / 4095;
        if (y0 < WAV_Y0) y0 = WAV_Y0;
        if (y1 < WAV_Y0) y1 = WAV_Y0;
        LCD_Line(x0, y0, x1, y1, YELLOW);
    }

    uint16_t vmin=4095, vmax=0;
    for (i=0; i<ADC_SAMPLE_COUNT; i++) {
        if (g_adcBuf[i]<vmin) vmin=g_adcBuf[i];
        if (g_adcBuf[i]>vmax) vmax=g_adcBuf[i];
    }
    sprintf(buf, "Vpp=%.2fV  %upts", (float)(vmax-vmin)*3.3f/4096.0f, ADC_SAMPLE_COUNT);
    LCD_String(5, 238, buf, 12, 0xFFE0, BLACK);
    LCD_String(140, 238, (char *)"Saved W25Q128", 12, 0x8410, BLACK);
}

/*===========================================================================
 * ADC 驱动 (TIM3 TRGO, 10ms)
 *===========================================================================*/
static void ADC1_PA2_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    __HAL_RCC_ADC1_CLK_ENABLE();
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.NbrOfDiscConversion = 0;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel = ADC_CHANNEL_2;
    ch.Rank = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
}

static void ADC1_StartSampling(void)
{
    g_adcIdx = 0;
    g_adcDone = 0;
    g_sampling = 1;
    memset((void *)g_adcBuf, 0, sizeof(g_adcBuf));
    HAL_ADC_Start_IT(&hadc1);
    TIM3_ADC_Trigger_Init();
}

static void ADC1_StopSampling(void)
{
    HAL_ADC_Stop_IT(&hadc1);
    TIM3->CR1 &= ~TIM_CR1_CEN;
    g_sampling = 0;
}

void ADC1_ConvCpltCallback(uint16_t val)
{
    if (!g_sampling) return;
    g_adcBuf[g_adcIdx++] = val;
    if (g_adcIdx >= ADC_SAMPLE_COUNT) {
        g_adcDone = 1;
        ADC1_StopSampling();
    }
}

/*===========================================================================
 * W25Q128 存储
 *===========================================================================*/
static void SaveToW25Q128(void)
{
    if (xW25Q128.FlagInit == 0) return;
    UART_Println("Saving to W25Q128...");
    W25Q128_WriteData(ADC_STORE_ADDR, (uint8_t *)g_adcBuf, ADC_SAMPLE_COUNT * 2);
    UART_Println("Done.");
}

/*===========================================================================
 * 主函数
 *===========================================================================*/
static void SystemClock_Config(void);

int main(void)
{
    uint8_t keyVal;

    HAL_Init();
    SystemClock_Config();

    /* UART */
    UART1_Init();
    UART_PrintSep();
    UART_Println("=== YueXinMiao + ADC + DSP ===");
    UART_Println("22 frames 128x64 -> ILI9341 2x");
    UART_Println("KEY1:ADC  KEY2:MoonCat  KEY3:DSP");
    UART_PrintSep();

    /* LCD */
    LCD_Init();
    LCD_DrawADCUI();
    UART_Println("LCD OK");

    /* Keys */
    key_init();

    /* W25Q128 */
    uint8_t w25q_ok = W25Q128_Init();
    UART_Println(w25q_ok ? "W25Q128 OK" : "W25Q128 FAIL");

    /* ADC1 */
    ADC1_PA2_Init();
    UART_Println("ADC1 PA2 OK");
    UART_PrintSep();

    /* 主循环 */
    while (1)
    {
        keyVal = key_scan(0);

        /* ================================================================
         * DSP 测试模式
         * ================================================================ */
        if (g_dspMode)
        {
            /* KEY1: 发送正弦波形 */
            if (keyVal == KEY1_PRES)
            {
                UART_PrintSep();
                UART_Println(">>> Sending SIN Waveform <<<");
                DSP_SendWaveform('S');
                UART_Println("[OK] SIN Sent.");
                UART_PrintSep();
            }

            /* KEY2: 发送余弦波形 */
            if (keyVal == KEY2_PRES)
            {
                UART_PrintSep();
                UART_Println(">>> Sending COS Waveform <<<");
                DSP_SendWaveform('C');
                UART_Println("[OK] COS Sent.");
                UART_PrintSep();
            }

            /* KEY3: 退出 DSP 模式 */
            if (keyVal == KEY3_PRES)
            {
                g_dspMode = 0;
                UART_Println("<<< Exit DSP Mode <<<");
                LCD_DrawADCUI();
            }

            HAL_Delay(20);
            continue;
        }

        /* ================================================================
         * 正常模式
         * ================================================================ */

        /* KEY2: 切换月薪喵模式 */
        if (keyVal == KEY2_PRES)
        {
            g_showMoonCat = !g_showMoonCat;
            if (g_showMoonCat) {
                UART_Println(">>> MoonCat Mode <<<");
                LCD_DrawMoonCatUI();
                g_mcatFrame = 0;
            } else {
                UART_Println(">>> ADC Mode <<<");
                LCD_DrawADCUI();
            }
        }

        /* KEY3: 进入 DSP 测试模式 */
        if (keyVal == KEY3_PRES && !g_sampling)
        {
            if (g_showMoonCat) {
                g_showMoonCat = 0;
            }

            UART_PrintSep();
            UART_Println(">>> DSP Sine/Cosine Test <<<");
            UART_Println("Generating 256-pt sequences...");

            /* 生成正余弦序列 */
            DSP_GenerateSequences();
            UART_Println("Sequences generated.");

            /* 计算统计量 */
            DSP_ComputeStatistics();
            UART_Println("Statistics computed.");

            /* LCD 显示结果 */
            DSP_DisplayResults();

            /* 通过串口打印统计结果 */
            {
                char buf[80];
                UART_PrintSep();
                UART_Println("--- SINE Statistics ---");
                sprintf(buf, "Max:  %+.4f", (double)g_sinMax);  UART_Println(buf);
                sprintf(buf, "Min:  %+.4f", (double)g_sinMin);  UART_Println(buf);
                sprintf(buf, "Mean: %+.4f", (double)g_sinMean); UART_Println(buf);
                sprintf(buf, "RMS:   %.4f", (double)g_sinRMS);  UART_Println(buf);
                UART_Println("--- COSINE Statistics ---");
                sprintf(buf, "Max:  %+.4f", (double)g_cosMax);  UART_Println(buf);
                sprintf(buf, "Min:  %+.4f", (double)g_cosMin);  UART_Println(buf);
                sprintf(buf, "Mean: %+.4f", (double)g_cosMean); UART_Println(buf);
                sprintf(buf, "RMS:   %.4f", (double)g_cosRMS);  UART_Println(buf);
                UART_PrintSep();
            }

            UART_Println("KEY1:Send SIN  KEY2:Send COS  KEY3:Exit");
            UART_PrintSep();

            g_dspMode = 1;
        }

        /* KEY1: ADC 采样 */
        if (keyVal == KEY1_PRES && !g_sampling)
        {
            if (g_showMoonCat) {
                g_showMoonCat = 0;
                LCD_DrawADCUI();
            }

            UART_PrintSep();
            UART_Println(">>> Sampling 256 pts <<<");
            ADC1_StartSampling();

            uint16_t last = 0;
            while (!g_adcDone) {
                if (g_adcIdx != last) {
                    last = g_adcIdx;
                    char buf[24];
                    sprintf(buf, "Sampling %u/256", last);
                    LCD_String(5, 75, buf, 16, YELLOW, BLACK);
                }
                HAL_Delay(5);
            }

            LCD_DrawWaveform();
            SaveToW25Q128();
            UART_Println("[OK] Done.");
            UART_PrintSep();
        }

        /* 月薪喵动画: 每帧直接覆盖写入 LCD GRAM, 不擦不叠 */
        if (g_showMoonCat && !g_sampling)
        {
            MoonCat_DrawFrame(g_mcatFrame);
            g_mcatFrame++;
            if (g_mcatFrame >= MOONCAT_FRAMES) g_mcatFrame = 0;
            HAL_Delay(20);
        }
        else
        {
            HAL_Delay(20);
        }
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

void Error_Handler(void) { __disable_irq(); while(1){} }
