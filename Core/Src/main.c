#include "main.h"
#include "gpio.h"
#include "uart.h"
#include "key.h"
#include "timer.h"
#include "bsp_LCD_ILI9341.h"
#include <stdio.h>
#include <math.h>

static void SystemClock_Config(void);

/* ---- 波形绘制参数 ---- */
#define WF_LEFT      45      /* 坐标系左边界 X       */
#define WF_RIGHT     225     /* 坐标系右边界 X       */
#define WF_TOP       185     /* 坐标系上边界 Y       */
#define WF_BOTTOM    295     /* 坐标系下边界 Y       */
#define WF_CYCLES    3       /* 锯齿波周期数          */
#define WF_VMAX      3.3f    /* 幅度最大值 (V)       */
#define WF_TMAX      30.0f   /* 时间轴最大值 (ms)    */



/**
 * @brief  绘制直角坐标系（带刻度线和标签）
 */
static void Draw_Coordinate(void)
{
    uint8_t i;
    int16_t x, y;
    char lbl[8];

    /* ---- 坐标轴外框 ---- */
    LCD_Line(WF_LEFT,  WF_TOP,    WF_RIGHT, WF_TOP,    WHITE);   /* 上边框  */
    LCD_Line(WF_LEFT,  WF_BOTTOM, WF_RIGHT, WF_BOTTOM, WHITE);   /* 下边框  */
    LCD_Line(WF_LEFT,  WF_TOP,    WF_LEFT,  WF_BOTTOM, WHITE);   /* 左边框  */
    LCD_Line(WF_RIGHT, WF_TOP,    WF_RIGHT, WF_BOTTOM, WHITE);   /* 右边框  */

    /* ---- Y 轴刻度（幅度: 0 ~ 3.3V, 每格 0.5V）---- */
    for (i = 0; i <= 7; i++)
    {
        y = WF_BOTTOM - (int16_t)((float)i * (WF_BOTTOM - WF_TOP) / 6.6f);
        if (y < WF_TOP || y > WF_BOTTOM) continue;

        /* 刻度短线 */
        LCD_Line(WF_LEFT - 5, y, WF_LEFT, y, WHITE);

        /* 水平虚线（网格） */
        for (x = WF_LEFT + 1; x < WF_RIGHT; x += 4)
            LCD_DrawPoint(x, y, GRAY);

        /* 标签 */
        if (i % 2 == 0)
        {
            snprintf(lbl, sizeof(lbl), "%.1f", (float)i * 0.5f);
            LCD_String(2, y - 6, lbl, 12, YELLOW, BLACK);
        }
    }

    /* ---- X 轴刻度（时间: 0 ~ 30ms, 每格 5ms）---- */
    for (i = 0; i <= 6; i++)
    {
        x = WF_LEFT + (int16_t)((float)i * (WF_RIGHT - WF_LEFT) / 6.0f);
        if (x < WF_LEFT || x > WF_RIGHT) continue;

        /* 刻度短线 */
        LCD_Line(x, WF_BOTTOM, x, WF_BOTTOM + 5, WHITE);

        /* 垂直虚线（网格） */
        for (y = WF_TOP + 1; y < WF_BOTTOM; y += 4)
            LCD_DrawPoint(x, y, GRAY);

        /* 标签 */
        snprintf(lbl, sizeof(lbl), "%d", i * 5);
        LCD_String(x - 8, WF_BOTTOM + 8, lbl, 12, YELLOW, BLACK);
    }

    /* ---- 轴标签 ---- */
    LCD_String(WF_RIGHT - 30, WF_TOP - 14, "V", 16, WHITE, BLACK);        /* Y 轴单位 */
    LCD_String(WF_RIGHT - 40, WF_BOTTOM + 8, "ms", 12, YELLOW, BLACK);    /* X 轴单位 */
    LCD_String(WF_LEFT + 60, WF_TOP - 14, "Sawtooth Wave", 16, CYAN, BLACK);
}

/**
 * @brief  绘制锯齿波波形（3 周期）
 *
 *         锯齿波公式: y(t) = Vmax * (t % T) / T
 *         周期 T = Tmax / Cycles
 *         逐像素绘制，保证连续
 */
static void Draw_Sawtooth(void)
{
    int16_t px, py;
    int16_t prev_px = 0, prev_py = 0;
    uint8_t first = 1;
    float period = WF_TMAX / (float)WF_CYCLES;      /* 单个周期时间 */
    float t, v;
    int16_t x_range = WF_RIGHT - WF_LEFT;
    int16_t y_range = WF_BOTTOM - WF_TOP;

    /* 逐列扫描，计算每个 X 像素对应的波形值 */
    for (px = WF_LEFT; px <= WF_RIGHT; px++)
    {
        /* 像素 X → 时间 t (ms) */
        t = (float)(px - WF_LEFT) * WF_TMAX / (float)x_range;

        /* 锯齿波: 取余周期内线性上升 */
        v = WF_VMAX * fmodf(t, period) / period;

        /* 幅度 v → 像素 Y (上小下大) */
        py = WF_BOTTOM - (int16_t)(v * (float)y_range / WF_VMAX);

        /* 边界裁剪 */
        if (py < WF_TOP)  py = WF_TOP;
        if (py > WF_BOTTOM) py = WF_BOTTOM;

        if (first)
        {
            prev_px = px;
            prev_py = py;
            first = 0;
        }
        else
        {
            /* 画线段连接相邻像素，保证锯齿波回落在相邻列完成 */
            LCD_Line(prev_px, prev_py, px, py, GREEN);
            prev_px = px;
            prev_py = py;
        }
    }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  TIM3_PWM_Init();        /* PA6: 50% PWM 输出        */
  TIM3_IC_Init();         /* PA7: 输入捕获 PA6 的波形  */
  TIM4_Breathe_Init();    /* PB1: 呼吸灯               */
  TIM2_Clock_Init();      /* TIM2: 数字钟 1s 定时      */
  key_init();
  UART1_Init();

  UART1_SendString("=== STM32F407 PWM Capture + LCD Clock ===\r\n");
  UART1_SendString("PA6 -> PWM OUT (50%%, 1kHz)\r\n");
  UART1_SendString("PA7 -> IC IN  (connect PA6--PA7)\r\n");
  UART1_SendString("LCD: Digital Clock (HH:MM:SS)\r\n\r\n");

  LCD_Init();
  LCD_SetDir(0);                                         /* 竖屏显示         */
  LCD_Fill(0, 0, 240, 320, BLACK);                       /* 黑色背景         */

  /* 上半部标题区域 */
  LCD_Fill(0, 0, 240, 159, DARKBLUE);                    /* 深蓝背景区分上下 */
  LCD_String(20, 10, "Digital Clock", 16, WHITE, DARKBLUE);

  /* 时钟下方显示名字 */
  LCD_String(55, 115, "jinchenghao", 24, YELLOW, DARKBLUE);

  /* 画一条分隔线 */
  LCD_Line(0, 159, 240, 159, CYAN);

  /* ---- 下半部：直角坐标 + 锯齿波波形 ---- */
  Draw_Coordinate();
  Draw_Sawtooth();

  uint32_t last_print = 0;
  static char lcd_buf[32];

  while (1)
  {
    uint8_t key = key_scan(0);

    if (key == KEY1_PRES)
    {
      UART1_SendString("KEY1 pressed\r\n");
    }
    else if (key == KEY2_PRES)
    {
      UART1_SendString("KEY2 pressed\r\n");
    }
    else if (key == KEY3_PRES)
    {
      UART1_SendString("KEY3 pressed\r\n");
    }

    /* ---- 数字钟 LCD 刷新（由 TIM2 ISR 触发）---- */
    if (g_clock_update)
    {
      g_clock_update = 0;

      /* 在 LCD 上半部居中显示 HH:MM:SS（字号 32） */
      snprintf(lcd_buf, sizeof(lcd_buf), "%02d:%02d:%02d",
               g_clock_hour, g_clock_min, g_clock_sec);

      /* 32 号字体每个字符宽约 16 像素, "HH:MM:SS" 共 8 字符 ≈ 128 像素 */
      /* 居中: X = (240 - 128) / 2 = 56, Y 在标题下方居中 */
      LCD_String(56, 60, lcd_buf, 32, GREEN, DARKBLUE);
    }

    /* ---- 每 500ms 串口打印捕获结果 ---- */
    if (HAL_GetTick() - last_print >= 500U)
    {
      last_print = HAL_GetTick();

      if (g_ic_result.valid)
      {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "Freq: %lu Hz | Period: %lu us | High: %lu us | Duty: %lu.%lu%%\r\n",
                 (unsigned long)g_ic_result.freq_hz,
                 (unsigned long)g_ic_result.period_us,
                 (unsigned long)g_ic_result.high_us,
                 (unsigned long)(g_ic_result.duty / 10),
                 (unsigned long)(g_ic_result.duty % 10));
        UART1_SendString(buf);
      }
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
