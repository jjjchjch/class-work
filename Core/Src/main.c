/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       ADC 定时触发 + LCD 实时波形
 *
 *              光敏电阻分压 → PC1 (ADC1_IN11)
 *              TIM2 TRGO 每 100ms 触发 ADC → DMA → LCD + 串口
 *              LCD 上半部: 采样数值显示
 *              LCD 下半部: 直角坐标 + 实时电压波形
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 ****************************************************************************************************
 */

#include "main.h"
#include "gpio.h"
#include "timer.h"
#include "adc.h"
#include "uart.h"
#include "bsp_LCD_ILI9341.h"
#include "bsp_W25Q128.h"
#include <stdio.h>
#include <string.h>

static void SystemClock_Config(void);

/* ==================== GBK 编码中文字符串 ====================
 * W25Q128 字库使用 GBK 编码, 源文件为 UTF-8 时需用字节数组
 * 否则 LCD 显示乱码
 * ========================================================== */
static const char STR_TITLE[]      = {0x41,0x44,0x43,0xB2,0xA8,0xD0,0xCE,0xCA,0xB5,0xD1,0xE9,0x00};  /* "ADC波形实验" */
static const char STR_PHOTO[]      = {0xB9,0xE2,0xC3,0xF4,0xB5,0xE7,0xD7,0xE8,0x20,0x50,0x43,0x31,0x00}; /* "光敏电阻 PC1" */
static const char STR_VOLTAGE[]    = {0xB5,0xE7,0xD1,0xB9,0x3A,0x00};                                /* "电压:" */

#define WF_LEFT       36      /* 坐标系左边界 */
#define WF_RIGHT      232     /* 坐标系右边界 (196像素宽, 19.6秒) */
#define WF_TOP        168     /* 坐标系上边界 (对应 3.3V) */
#define WF_BOTTOM     308     /* 坐标系下边界 (对应 0V) */
#define WF_BUF_SIZE   ((WF_RIGHT) - (WF_LEFT) + 1)  /* 197 个采样点 */

static uint16_t g_wave_buf[WF_BUF_SIZE];  /* 波形数据缓冲区 (ADC原始值) */
static uint16_t g_wave_idx = 0;           /* 当前写入位置 */
static uint16_t g_wave_cnt = 0;           /* 已采集点数 (≤ WF_BUF_SIZE) */

/* ==================== 波形绘制函数 ==================== */

/**
 * @brief  绘制直角坐标系 (仅执行一次)
 */
static void Wave_DrawAxis(void)
{
    uint8_t i;
    int16_t y;
    char lbl[8];

    /* ---- 坐标轴外框 ---- */
    LCD_Line(WF_LEFT,  WF_TOP,    WF_RIGHT, WF_TOP,    WHITE);
    LCD_Line(WF_LEFT,  WF_BOTTOM, WF_RIGHT, WF_BOTTOM, WHITE);
    LCD_Line(WF_LEFT,  WF_TOP,    WF_LEFT,  WF_BOTTOM, WHITE);
    LCD_Line(WF_RIGHT, WF_TOP,    WF_RIGHT, WF_BOTTOM, WHITE);

    /* ---- Y 轴刻度: 0 ~ 3.3V, 每 1.0V 一格 ---- */
    for (i = 0; i <= 3; i++)
    {
        y = WF_BOTTOM - (int16_t)((float)i * (float)(WF_BOTTOM - WF_TOP) / 3.3f);
        if (y < WF_TOP || y > WF_BOTTOM) continue;

        LCD_Line(WF_LEFT - 4, y, WF_LEFT, y, WHITE);

        /* 水平虚线(网格) */
        for (int16_t x = WF_LEFT + 1; x < WF_RIGHT; x += 4)
            LCD_DrawPoint(x, y, GRAY);

        snprintf(lbl, sizeof(lbl), "%d", i);
        LCD_String(WF_LEFT - 30, y - 6, lbl, 12, YELLOW, BLACK);
    }

    /* ---- 单位标注 ---- */
    LCD_String(WF_LEFT - 28, WF_TOP - 16, "V", 12, WHITE, BLACK);
    LCD_String(WF_RIGHT - 35, WF_BOTTOM + 4, "t(s)", 12, YELLOW, BLACK);
}

/**
 * @brief  添加新采样点并刷新波形
 * @param  adc_val: 12位 ADC 值
 */
static void Wave_Update(uint16_t adc_val)
{
    int16_t prev_x, prev_y, cur_x, cur_y;
    uint16_t i;
    float v;

    /* 存入循环缓冲区 */
    g_wave_buf[g_wave_idx] = adc_val;
    g_wave_idx++;
    if (g_wave_idx >= WF_BUF_SIZE) g_wave_idx = 0;
    if (g_wave_cnt < WF_BUF_SIZE)  g_wave_cnt++;

    /* 清除波形区域 */
    LCD_Fill(WF_LEFT + 1, WF_TOP + 1, WF_RIGHT - 1, WF_BOTTOM - 1, BLACK);

    /* 绘制所有波形点 */
    for (i = 0; i < g_wave_cnt; i++)
    {
        /* 从缓冲区取数据: 最旧的在 (g_wave_idx - g_wave_cnt) 位置 */
        uint16_t idx;
        if (g_wave_cnt < WF_BUF_SIZE)
            idx = i;     /* 缓冲区未满, 从头开始 */
        else
            idx = (g_wave_idx + i) % WF_BUF_SIZE;  /* 循环 */

        v = (float)g_wave_buf[idx] * (3.3f / 4096.0f);

        cur_x = WF_LEFT + (int16_t)i;
        cur_y = WF_BOTTOM - (int16_t)(v / 3.3f * (float)(WF_BOTTOM - WF_TOP));

        if (cur_y < WF_TOP)  cur_y = WF_TOP;
        if (cur_y > WF_BOTTOM) cur_y = WF_BOTTOM;

        if (i > 0)
        {
            LCD_Line(prev_x, prev_y, cur_x, cur_y, GREEN);
        }
        prev_x = cur_x;
        prev_y = cur_y;
    }
}

/* ==================== 主函数 ==================== */

int main(void)
{
    static char strTemp[30];
    uint16_t adc_val = 0;
    uint32_t last_lcd = 0;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* ---- 清零波形缓冲区 ---- */
    memset(g_wave_buf, 0, sizeof(g_wave_buf));

    /* ---- UART 初始化 ---- */
    UART1_Init();
    UART1_SendString("=== ADC Timer Trigger: TIM2(100ms) -> ADC1_IN11(PC1) ===\r\n");
    UART1_SendString("LCD: upper=value, lower=waveform | PA6=PWM wave out\r\n\r\n");

    /* ---- LCD 初始化 ---- */
    LCD_Init();
    LCD_SetDir(0);
    LCD_Fill(0, 0, 240, 320, BLACK);

    /* ---- 标题栏 ---- */
    LCD_Fill(0, 0, 240, 40, DARKBLUE);
    LCD_String(30, 8, (char *)STR_TITLE, 16, WHITE, DARKBLUE);
    LCD_Line(0, 40, 240, 40, CYAN);

    /* ---- 上半部: 光敏电阻采样数值 ---- */
    LCD_String(5,  50,  (char *)STR_PHOTO,   24, BLACK, GREEN);
    LCD_String(5,  82,  "ADC:",  24, CYAN,  BLACK);
    LCD_String(5,  115, (char *)STR_VOLTAGE, 24, CYAN,  BLACK);

    /* ---- 分隔线 ---- */
    LCD_Line(5, 152, 235, 152, GRAY);

    /* ---- 下半部: 波形坐标系 ---- */
    Wave_DrawAxis();

    /* ---- 底部 ---- */
    LCD_Line(0, 315, 240, 315, CYAN);

    /* ---- 启动 TIM2 (100ms TRGO) ---- */
    TIM2_TRGO_Init();

    /* ---- 初始化 W25Q128 (中文字库) ---- */
    W25Q128_Init();

    /* ---- 初始化 ADC + DMA ---- */
    adc_init();
    adc_start_dma();

    while (1)
    {
        /* 检查 DMA 是否有新数据 (每 100ms 一次) */
        if (g_adc_new_data)
        {
            g_adc_new_data = 0;
            adc_val = g_adc_dma_buf;

            /* ---- 串口发送 ADC 值 (供示波器显示波形) ---- */
            UART1_SendADC(adc_val);

            /* ---- 刷新 LCD 波形 ---- */
            Wave_Update(adc_val);
        }

        /* LCD 数值区域刷新 (每 200ms) */
        if (HAL_GetTick() - last_lcd >= 200U)
        {
            last_lcd = HAL_GetTick();

            float v = adc_get_voltage_v(adc_val);

            sprintf(strTemp, "%4d    ", adc_val);
            LCD_String(80, 82, strTemp, 24, GREEN, BLACK);

            sprintf(strTemp, "%1.2f V    ", (double)v);
            LCD_String(80, 115, strTemp, 24, GREEN, BLACK);
        }
    }
}

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
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
