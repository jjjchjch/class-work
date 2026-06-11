/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       ADC 裸测 — PC1 光敏电阻采样, 串口打印
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 ****************************************************************************************************
 */

#include "main.h"
#include "gpio.h"
#include "adc.h"
#include "uart.h"
#include <stdio.h>

static void SystemClock_Config(void);

int main(void)
{
    uint16_t adc_val;
    uint32_t count = 0;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    UART1_Init();
    UART1_SendString("\r\n=== ADC Test: PC1(ADC1_IN11) ===\r\n\r\n");

    adc_init();
    UART1_SendString("[OK] ADC init done\r\n");

    while (1)
    {
        adc_val = adc_read();

        char buf[64];
        sprintf(buf, "[%4lu] raw=%4d  V=%.2fV\r\n",
                count++, adc_val, (double)adc_get_voltage_v(adc_val));
        UART1_SendString(buf);

        HAL_Delay(500);
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
