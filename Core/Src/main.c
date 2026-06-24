/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       BMP280 双轴波形显示 (温度红色 / 气压绿色)
 *
 *              LCD 布局 (240×320 竖屏):
 *              ┌──────────────────────────┐ y=0
 *              │    BMP280 温度气压波形     │ 标题栏 (蓝底)
 *              ├──────────────────────────┤ y=25
 *              │ 50℃─┤                    │
 *              │      │    波形区域        │
 *              │ 25℃─┤   (红色=温度)      ├─105kPa
 *              │      │   (绿色=气压)      │
 *              │  0℃─┤                    ├─100kPa
 *              ├──────────────────────────┤ y=295
 *              │ T:25.3℃  P:101.3kPa     │ 数值显示
 *              └──────────────────────────┘ y=319
 ****************************************************************************************************
 */

#include "main.h"
#include "uart.h"
#include "delay.h"
#include "key.h"
#include "bsp_LCD_ILI9341.h"
#include "bmp280.h"
#include <stdio.h>
#include <string.h>

/* ---- 颜色 ---- */
#define BLACK   0x0000
#define WHITE   0xFFFF
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x7FFF
#define YELLOW  0xFFE0
#define DGRAY   0x39E7

/* ---- 串口缓冲 ---- */
char g_uartBuf[128];

/* ---- 波形缓冲: 200点环形, 对应 LCD 绘图宽度 ---- */
#define BUF_SIZE  200
static float g_tBuf[BUF_SIZE];
static float g_pBuf[BUF_SIZE];
static int   g_bufIdx = 0;
static int   g_bufCnt = 0;

/* ---- 坐标轴范围 ---- */
static float g_tMin = 0.0f,  g_tMax = 50.0f;
static float g_pMin = 95.0f, g_pMax = 105.0f;

/* ---- 绘图区域 ---- */
#define PLOT_X0   36
#define PLOT_Y0   28
#define PLOT_X1   200
#define PLOT_Y1   288
#define PLOT_W    (PLOT_X1 - PLOT_X0)
#define PLOT_H    (PLOT_Y1 - PLOT_Y0)

/*===========================================================================
 * 系统时钟: HSI 16MHz
 *===========================================================================*/
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

/*===========================================================================
 * 绘制静态坐标系
 *===========================================================================*/
static void LCD_DrawAxes(void)
{
    int16_t i;
    char buf[16];

    /* 标题栏 */
    LCD_Fill(0, 0, 239, 24, BLUE);
    LCD_String(50, 3, (char *)"BMP280 Wave", 16, WHITE, BLUE);

    /* 图表区域背景 */
    LCD_Fill(PLOT_X0 - 1, PLOT_Y0 - 1, PLOT_X1 + 1, PLOT_Y1 + 1, DGRAY);
    LCD_Fill(PLOT_X0, PLOT_Y0, PLOT_X1, PLOT_Y1, BLACK);

    /* 水平网格线 */
    for (i = 0; i <= 4; i++)
    {
        int16_t y = PLOT_Y0 + (int16_t)((uint32_t)i * PLOT_H / 4);
        LCD_Line(PLOT_X0, y, PLOT_X1, y, DGRAY);
    }

    /* 左轴 (温度, 红色): 横排标签 */
    for (i = 0; i <= 4; i++)
    {
        int16_t y = PLOT_Y0 + (int16_t)((uint32_t)i * PLOT_H / 4);
        float val = g_tMax - (g_tMax - g_tMin) * i / 4.0f;
        sprintf(buf, "%.0f", (double)val);
        LCD_String(2, y - 6, buf, 12, RED, BLACK);
    }
    LCD_String(4, PLOT_Y0 - 14, (char *)"T(C)", 12, RED, BLACK);

    /* 右轴 (气压, 绿色): 横排标签, 与左轴对称 */
    for (i = 0; i <= 4; i++)
    {
        int16_t y = PLOT_Y0 + (int16_t)((uint32_t)i * PLOT_H / 4);
        float val = g_pMax - (g_pMax - g_pMin) * i / 4.0f;
        sprintf(buf, "%.1f", (double)val);
        LCD_String(PLOT_X1 + 3, y - 6, buf, 12, GREEN, BLACK);
    }
    LCD_String(PLOT_X1 + 3, PLOT_Y0 - 14, (char *)"kPa", 12, GREEN, BLACK);

    /* 底部图例 */
    LCD_Fill(0, PLOT_Y1 + 2, 239, 319, BLACK);
    LCD_Fill(4, PLOT_Y1 + 8, 14, PLOT_Y1 + 12, RED);
    LCD_String(18, PLOT_Y1 + 5, (char *)"Temp(C)", 12, WHITE, BLACK);
    LCD_Fill(100, PLOT_Y1 + 8, 110, PLOT_Y1 + 12, GREEN);
    LCD_String(114, PLOT_Y1 + 5, (char *)"Press(kPa)", 12, WHITE, BLACK);
}

/*===========================================================================
 * 刷新波形: 擦旧线, 画新线 (双色)
 *===========================================================================*/
static void LCD_DrawWave(void)
{
    int16_t i;
    int16_t x0, y0_t, y0_p, x1, y1_t, y1_p;
    int start, count;

    if (g_bufCnt < 2) return;

    count = (g_bufCnt < BUF_SIZE) ? g_bufCnt : BUF_SIZE;
    start = (g_bufCnt < BUF_SIZE) ? 0 : g_bufIdx;

    /* 擦除上一帧 */
    LCD_Fill(PLOT_X0, PLOT_Y0, PLOT_X1, PLOT_Y1, BLACK);

    /* 重画网格 */
    for (i = 1; i <= 3; i++)
    {
        int16_t y = PLOT_Y0 + (int16_t)((uint32_t)i * PLOT_H / 4);
        LCD_Line(PLOT_X0, y, PLOT_X1, y, DGRAY);
    }

    /* 画波形折线 */
    for (i = 1; i < count; i++)
    {
        int idx0 = (start + i - 1) % BUF_SIZE;
        int idx1 = (start + i) % BUF_SIZE;

        x0 = PLOT_X0 + (int16_t)((uint32_t)(i - 1) * PLOT_W / (count - 1));
        x1 = PLOT_X0 + (int16_t)((uint32_t)i       * PLOT_W / (count - 1));

        /* 温度 (红色) */
        y0_t = PLOT_Y0 + (int16_t)((double)(g_tMax - g_tBuf[idx0]) * PLOT_H / (g_tMax - g_tMin));
        y1_t = PLOT_Y0 + (int16_t)((double)(g_tMax - g_tBuf[idx1]) * PLOT_H / (g_tMax - g_tMin));
        if (y0_t < PLOT_Y0) y0_t = PLOT_Y0;
        if (y1_t < PLOT_Y0) y1_t = PLOT_Y0;
        if (y0_t > PLOT_Y1) y0_t = PLOT_Y1;
        if (y1_t > PLOT_Y1) y1_t = PLOT_Y1;
        LCD_Line(x0, y0_t, x1, y1_t, RED);

        /* 气压 (绿色) */
        y0_p = PLOT_Y0 + (int16_t)((double)(g_pMax - g_pBuf[idx0]) * PLOT_H / (g_pMax - g_pMin));
        y1_p = PLOT_Y0 + (int16_t)((double)(g_pMax - g_pBuf[idx1]) * PLOT_H / (g_pMax - g_pMin));
        if (y0_p < PLOT_Y0) y0_p = PLOT_Y0;
        if (y1_p < PLOT_Y0) y1_p = PLOT_Y0;
        if (y0_p > PLOT_Y1) y0_p = PLOT_Y1;
        if (y1_p > PLOT_Y1) y1_p = PLOT_Y1;
        LCD_Line(x0, y0_p, x1, y1_p, GREEN);
    }
}

/*===========================================================================
 * 更新底部数值
 *===========================================================================*/
static void LCD_UpdateValues(float temp, float press)
{
    char buf[32];
    LCD_Fill(0, PLOT_Y1 + 3, 239, 319, BLACK);

    sprintf(buf, "T:%.1f C", (double)temp);
    LCD_String(10, PLOT_Y1 + 6, buf, 16, RED, BLACK);

    sprintf(buf, "P:%.2f kPa", (double)press);
    LCD_String(120, PLOT_Y1 + 6, buf, 16, GREEN, BLACK);
}

/*===========================================================================
 * 主函数
 *===========================================================================*/
int main(void)
{
    uint16_t tick = 0;
    float tVal, pVal;

    HAL_Init();
    SystemClock_Config();

    /* ---- UART ---- */
    UART1_Init();
    UART1_SendString("\r\n=== BMP280 Wave ===\r\n");
    UART1_SendString("CH1:Temp(C) CH2:Press(kPa)\r\n\r\n");

    /* ---- LED ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* ---- LCD ---- */
    LCD_Init();
    LCD_SetDir(0);
    LCD_Fill(0, 0, 240, 320, BLACK);

    /* ---- BMP280 ---- */
    Bmp280Init();

    /* ---- 画坐标系 ---- */
    LCD_DrawAxes();

    /* ---- 主循环 ---- */
    while (1)
    {
        if (tick % 10 == 0)
        {
            bmp280_GetValue();
            tVal = Bmp280Data.T;
            pVal = Bmp280Data.P / 1000.0f;   /* Pa → kPa */

            /* 存波形 */
            g_tBuf[g_bufIdx] = tVal;
            g_pBuf[g_bufIdx] = pVal;
            g_bufIdx = (g_bufIdx + 1) % BUF_SIZE;
            if (g_bufCnt < BUF_SIZE) g_bufCnt++;

            /* 动态调整 Y 轴范围 */
            if (tVal < g_tMin - 1.0f || tVal > g_tMax + 1.0f ||
                pVal < g_pMin - 1.0f || pVal > g_pMax + 1.0f)
            {
                g_tMin = (float)((int)(tVal / 5.0f) * 5);
                g_tMax = g_tMin + 50.0f;
                g_pMin = (float)((int)(pVal - 3.0f));
                g_pMax = g_pMin + 10.0f;
                LCD_DrawAxes();
            }

            /* 串口 */
            sprintf(g_uartBuf, "%.1f,%.3f\r\n", (double)tVal, (double)pVal);
            UART1_SendString(g_uartBuf);

            /* LCD 波形 + 数值 */
            LCD_DrawWave();
            LCD_UpdateValues(tVal, pVal);
        }

        delay_ms(10);
        tick++;
        if (tick == 20) { tick = 0; HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5); }
    }
}

/* 桩函数 */
ADC_HandleTypeDef hadc1 = {0};
void ADC1_ConvCpltCallback(uint16_t val) { (void)val; }
void Error_Handler(void) { __disable_irq(); while (1) {} }

