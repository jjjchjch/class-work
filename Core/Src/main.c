/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       TIM4 TRGO → ADC2 中断采样 → 串口打印
 *              光敏电阻分压 → PC1 (ADC2_IN11), 10ms 间隔
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 ****************************************************************************************************
 */

#include "main.h"
#include "gpio.h"
#include "uart.h"
#include "adc.h"
#include "timer.h"
#include <stdio.h>

static void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* ---- UART1 初始化 (DMA 发送) ---- */
    UART1_Init();
    UART1_DMA_Init();

    /* 先发一条确认 UART 工作 */
    UART1_DMA_SendString("\r\n=== ADC2 TIM4 TRGO Test ===\r\n");
    while (UART1_DMA_IsBusy()) {}

    /* ---- TIM3 TRGO 10ms → ADC2 外部触发 (EXTSEL=8) ---- */
    TIM3_ADC_Trigger_Init();

    /* ---- ADC2 中断模式 ---- */
    adc2_init();

    UART1_DMA_SendString("ADC2 started, waiting...\r\n");
    while (UART1_DMA_IsBusy()) {}

    while (1)
    {
        /* 每个采样值实时打印 (10ms一次) */
        if (adc2_new_val)
        {
            adc2_new_val = 0;
            char buf[16];
            int len = snprintf(buf, sizeof(buf), "%d\r\n", adc2_latest);
            if (len > 0) {
                UART1_DMA_SendString(buf);
                while (UART1_DMA_IsBusy()) {}
            }
        }

        /* 每 100 个值打印平均值 */
        if (adc2_transfer_done == 1)
        {
            adc2_transfer_done = 0;

            float avg_v = adc2_get_average_v();
            float avg_raw = avg_v / (3.3f / 4096.0f);

            char buf[64];
            sprintf(buf, "--- Batch %lu: Avg=%.0f  Vin=%.3fV ---\r\n",
                    (unsigned long)adc2_transfer_cnt,
                    (double)avg_raw, (double)avg_v);
            UART1_DMA_SendString(buf);
            while (UART1_DMA_IsBusy()) {}
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
