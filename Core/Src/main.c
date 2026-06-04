#include "main.h"
#include "gpio.h"
#include "uart.h"
#include "key.h"
#include "timer.h"
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
  key_init();
  UART1_Init();

  UART1_SendString("=== STM32F407 PWM Capture Demo ===\r\n");
  UART1_SendString("PA6 -> PWM OUT (50%%, 1kHz)\r\n");
  UART1_SendString("PA7 -> IC IN  (connect PA6--PA7)\r\n");
  UART1_SendString("Capture result prints every 500ms\r\n\r\n");

  uint32_t last_print = 0;

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

    /* 每 500ms 打印一次捕获结果 */
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
      else
      {
        UART1_SendString("Waiting for capture... (connect PA6 to PA7)\r\n");
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
