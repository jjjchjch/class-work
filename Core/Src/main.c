#include "main.h"
#include "gpio.h"
#include "uart.h"
#include "key.h"
#include "timer.h"
#include "bsp_LCD_ILI9341.h"
#include <stdio.h>

static void SystemClock_Config(void);

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

  /* 画一条分隔线 */
  LCD_Line(0, 159, 240, 159, CYAN);

  /* 下半部显示 PWM 捕获信息标题 */
  LCD_String(20, 170, "PWM Capture", 16, YELLOW, BLACK);

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
