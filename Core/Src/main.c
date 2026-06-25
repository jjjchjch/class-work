/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       AD9833 DDS 信号发生器实验 (软件模拟 SPI)
 *              AD9833_FSYNC => PB12
 *              AD9833_CLK   => PB13
 *              AD9833_DATA  => PB15
 ****************************************************************************************************
 */

#include "main.h"
#include "delay.h"
#include "uart.h"
#include "key.h"
#include "bsp_LCD_ILI9341.h"
#include "bsp_AD9833.h"
#include <stdio.h>
#include <math.h>

/* ---- 颜色 ---- */
#define BLACK   0x0000
#define WHITE   0xFFFF
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x7FFF
#define YELLOW  0xFFE0

/* ---- 波形定义 ---- */
typedef enum {
    WAVE_SIN = 0,
    WAVE_TRI,
    WAVE_SQR,
    WAVE_NUM
} WaveType_t;

static const unsigned int g_waveReg[WAVE_NUM] = {
    AD9833_OUT_SINUS,
    AD9833_OUT_TRIANGLE,
    AD9833_OUT_MSB
};
static const char *g_waveName[WAVE_NUM] = {
    "SIN ",
    "TRI ",
    "SQR "
};

/* ---- 频率档位 (Hz) ---- */
static const double g_freqList[] = {
    100.0, 500.0, 1000.0, 2000.0, 5000.0,
    10000.0, 50000.0, 100000.0, 500000.0, 1000000.0
};
#define FREQ_NUM  (sizeof(g_freqList) / sizeof(g_freqList[0]))

/* ---- 波形显示区域参数 (LCD 下部, 信号源/示波器风格) ---- */
#define WV_X0     8                 /* 显示区左边界 */
#define WV_X1     232               /* 显示区右边界 */
#define WV_Y0     150               /* 显示区上边界 */
#define WV_Y1     264               /* 显示区下边界 */
#define WV_YC     207               /* 中心线 Y */
#define WV_AMP    50                /* 振幅 (像素) */
#define WV_N      180               /* 采样点数 */
#define WV_CYC    2                 /* 显示周期数 */
#define WV_GRID   20                /* 网格间距 (像素) */

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
 * 频率格式化: <1kHz 显示 Hz, <1MHz 显示 kHz, 否则 MHz
 *===========================================================================*/
static void format_freq(double f, char *buf, size_t bufsize)
{
    if (f < 1000.0)
        snprintf(buf, bufsize, "%.0f Hz", f);
    else if (f < 1000000.0)
        snprintf(buf, bufsize, "%.3f kHz", f / 1000.0);
    else
        snprintf(buf, bufsize, "%.3f MHz", f / 1000000.0);
}

/*===========================================================================
 * 计算归一化波形值 (-1.0 .. +1.0)
 *===========================================================================*/
static double wave_sample(WaveType_t wave, double t)
{
    double ph;
    switch (wave)
    {
        case WAVE_SIN:
            return sin(2.0 * M_PI * WV_CYC * t);
        case WAVE_TRI:
            /* 三角波: 上升 1/2 周期, 下降 1/2 周期 */
            ph = fmod(t * WV_CYC, 1.0) * 2.0;       /* 0..2 */
            return (ph < 1.0) ? (2.0 * ph - 1.0) : (3.0 - 2.0 * ph);
        case WAVE_SQR:
        default:
            /* 方波: 高半周期, 低半周期 */
            return (fmod(t * WV_CYC, 1.0) < 0.5) ? 1.0 : -1.0;
    }
}

/*===========================================================================
 * 绘制示波器风格背景: 黑色屏幕 + 绿色网格 + 边框
 *===========================================================================*/
static void lcd_draw_scope_bg(void)
{
    int x, y;

    /* 黑色屏幕背景 */
    LCD_Fill(WV_X0, WV_Y0, WV_X1, WV_Y1, BLACK);

    /* 绿色网格: 垂直线 */
    for (x = WV_X0 + WV_GRID; x < WV_X1; x += WV_GRID)
    {
        for (y = WV_Y0 + 2; y < WV_Y1 - 2; y += 4)
            LCD_DrawPoint(x, y, GREEN);
    }
    /* 绿色网格: 水平线 */
    for (y = WV_Y0 + WV_GRID; y < WV_Y1; y += WV_GRID)
    {
        for (x = WV_X0 + 2; x < WV_X1 - 2; x += 4)
            LCD_DrawPoint(x, y, GREEN);
    }

    /* 中心十字线 (实线, 略亮) */
    LCD_Line(WV_X0, WV_YC, WV_X1, WV_YC, GREEN);
    x = (WV_X0 + WV_X1) / 2;
    LCD_Line(x, WV_Y0, x, WV_Y1, GREEN);

    /* 边框 */
    LCD_Line(WV_X0,     WV_Y0,     WV_X1,     WV_Y0,     YELLOW);
    LCD_Line(WV_X0,     WV_Y1,     WV_X1,     WV_Y1,     YELLOW);
    LCD_Line(WV_X0,     WV_Y0,     WV_X0,     WV_Y1,     YELLOW);
    LCD_Line(WV_X1,     WV_Y0,     WV_X1,     WV_Y1,     YELLOW);
}

/*===========================================================================
 * 在 LCD 上绘制波形示意 (信号源风格, 固定显示 WV_CYC 个周期)
 *===========================================================================*/
static void lcd_draw_wave(WaveType_t wave)
{
    int i, x_prev, y_prev, x, y;

    /* 重绘示波器背景 */
    lcd_draw_scope_bg();

    /* 计算起始点 */
    x_prev = WV_X0;
    y_prev = WV_YC - (int)(wave_sample(wave, 0.0) * WV_AMP);

    /* 逐点连线 (青色波形) */
    for (i = 1; i <= WV_N; i++)
    {
        double t = (double)i / WV_N;
        x = WV_X0 + (WV_X1 - WV_X0) * i / WV_N;
        y = WV_YC - (int)(wave_sample(wave, t) * WV_AMP);
        LCD_Line(x_prev, y_prev, x, y, CYAN);
        x_prev = x;
        y_prev = y;
    }
}

/*===========================================================================
 * LCD 刷新: 标题 / 当前波形 / 当前频率 / 波形示意 / 操作提示
 *===========================================================================*/
static void lcd_update(WaveType_t wave, uint8_t freq_idx)
{
    char line[32];
    char fbuf[24];

    /* 标题栏 */
    LCD_Fill(0, 0, 239, 28, BLUE);
    LCD_String(40, 4, (char *)"AD9833 DDS", 20, WHITE, BLUE);

    /* 波形 + 频率 (并排显示, 像信号源面板) */
    LCD_Fill(0, 32, 239, 78, BLACK);
    snprintf(line, sizeof(line), "Wave: %s", g_waveName[wave]);
    LCD_String(8, 38, line, 20, YELLOW, BLACK);

    format_freq(g_freqList[freq_idx], fbuf, sizeof(fbuf));
    snprintf(line, sizeof(line), "Freq: %s", fbuf);
    LCD_String(8, 58, line, 20, GREEN, BLACK);

    /* 波形示意 (LCD 下部, 信号源风格) */
    lcd_draw_wave(wave);

    /* 操作提示 */
    LCD_Fill(0, 270, 239, 319, BLUE);
    LCD_String(8, 280, (char *)"K1:Wave  K2:+  K3:-", 14, WHITE, BLUE);
    LCD_String(8, 300, (char *)"DDS Signal Generator", 12, YELLOW, BLUE);
}

/*===========================================================================
 * 主程序: 按键控制 AD9833 输出波形与频率
 *===========================================================================*/
int main(void)
{
    WaveType_t wave = WAVE_SIN;
    uint8_t    freq_idx = 2;            /* 默认 1 kHz 正弦 */
    uint8_t    key;

    HAL_Init();
    SystemClock_Config();

    /* ---- UART1 调试输出 ---- */
    UART1_Init();
    UART1_SendString("\r\n========== AD9833 DDS ==========\r\n");

    /* ---- LED1 (PC5) 心跳 ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* ---- 按键 ---- */
    key_init();

    /* ---- LCD 显示 ---- */
    LCD_Init();
    LCD_SetDir(0);                      /* 0-竖屏, 1-横屏 */
    LCD_Fill(0, 0, 240, 320, WHITE);

    /* ---- AD9833 初始化并按默认参数输出 ---- */
    AD9833_Init();
    AD9833_Setup(AD9833_REG_FREQ0, g_freqList[freq_idx],
                 AD9833_REG_PHASE0, 0, g_waveReg[wave]);

    lcd_update(wave, freq_idx);

    /* ---- 主循环: 扫描按键, 200ms LED 心跳 ---- */
    while (1)
    {
        key = key_scan(0);              /* 单次触发模式 */
        if (key == KEY1_PRES)           /* 切换波形 */
        {
            wave = (WaveType_t)((wave + 1) % WAVE_NUM);
            AD9833_Setup(AD9833_REG_FREQ0, g_freqList[freq_idx],
                         AD9833_REG_PHASE0, 0, g_waveReg[wave]);
            lcd_update(wave, freq_idx);
        }
        else if (key == KEY2_PRES)      /* 频率 + */
        {
            if (freq_idx < FREQ_NUM - 1) freq_idx++;
            AD9833_Setup(AD9833_REG_FREQ0, g_freqList[freq_idx],
                         AD9833_REG_PHASE0, 0, g_waveReg[wave]);
            lcd_update(wave, freq_idx);
        }
        else if (key == KEY3_PRES)      /* 频率 - */
        {
            if (freq_idx > 0) freq_idx--;
            AD9833_Setup(AD9833_REG_FREQ0, g_freqList[freq_idx],
                         AD9833_REG_PHASE0, 0, g_waveReg[wave]);
            lcd_update(wave, freq_idx);
        }

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5);
        delay_ms(50);
    }
}

/* 桩函数 */
ADC_HandleTypeDef hadc1 = {0};
void ADC1_ConvCpltCallback(uint16_t val) { (void)val; }
void Error_Handler(void) { __disable_irq(); while (1) {} }

