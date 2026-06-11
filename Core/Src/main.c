/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       ADC→DAC 光敏实验 — 定时采样 + PA4 输出
 *
 *              光敏电阻分压 → PC1 (ADC1_IN11), 每 100ms 采样一次
 *              ADC 值 → DAC CH1 → PA4 电压输出
 *              示波器接 PA4 观察波形随光强变化
 *              串口同步打印 ADC 值和电压
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 ****************************************************************************************************
 */

#include "main.h"
#include "gpio.h"
#include "adc.h"
#include "uart.h"
#include "dac.h"
#include <stdio.h>

static void SystemClock_Config(void);

int main(void)
{
    uint16_t adc_val;
    uint32_t count = 0;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* ---- UART ---- */
    UART1_Init();
    UART1_SendString("\r\n========================================\r\n");
    UART1_SendString("  ADC->DAC 光敏实验 (定时采样)\r\n");
    UART1_SendString("  PC1(ADC) -> PA4(DAC) 电压跟随\r\n");
    UART1_SendString("  示波器接 PA4 观察光强变化\r\n");
    UART1_SendString("========================================\r\n\r\n");

    /* ---- 初始化 DAC (PA4 CH1, 软件触发) ---- */
    DAC_Init();

    /* ---- 初始化 ADC (PC1, CH11) ---- */
    adc_init();

    UART1_SendString("[OK] 启动完成, 每100ms采样一次\r\n\r\n");

    while (1)
    {
        /* ADC 采样 */
        adc_val = adc_read();

        /* DAC 输出 → PA4 电压 = adc_val × 3.3V / 4096 */
        DAC_SetCh1(adc_val);

        /* 串口打印 */
        float v = adc_get_voltage_v(adc_val);
        char buf[64];
        sprintf(buf, "[%4lu] ADC=%4d  Vin=%.2fV  PA4=%.2fV\r\n",
                count++, adc_val, (double)v, (double)v);
        UART1_SendString(buf);

        HAL_Delay(100);  /* 100ms 定时采样间隔 */
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
