/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       DAC1 DMA 锯齿波 — TIM7 1ms 触发 + PA4 输出
 *
 *              锯齿波: 128 点 (0→4064, 步长 32)
 *              TIM7 TRGO 每 1ms 触发一次 DAC1 转换
 *              DMA1 Stream5 CH7 循环搬运数据
 *              PA4 输出锯齿波 (128ms 周期)
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 ****************************************************************************************************
 */

#include "main.h"
#include "gpio.h"
#include "uart.h"
#include "dac.h"
#include "timer.h"
#include <stdio.h>

static void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* ---- UART1 (DMA 发送) ---- */
    UART1_Init();
    UART1_DMA_Init();

    UART1_DMA_SendString("\r\n=== DAC1 Sawtooth Wave ===\r\n");
    while (UART1_DMA_IsBusy()) {}
    UART1_DMA_SendString("TIM7 1ms TRGO, 128 points\r\n");
    while (UART1_DMA_IsBusy()) {}

    /* ---- TIM7 TRGO 1ms → DAC1 触发 ---- */
    TIM7_DAC_Trigger_Init();

    /* ---- DAC1 DMA 锯齿波初始化 ---- */
    DAC_SAW_Init();

    /* 串口打印前 10 个锯齿波数据 */
    UART1_DMA_SendString("Sawtooth data[0..9]: ");
    while (UART1_DMA_IsBusy()) {}
    for (int i = 0; i < 10; i++)
    {
        char buf[8];
        int len = snprintf(buf, sizeof(buf), "%d ", dac_saw_buf[i]);
        if (len > 0) {
            UART1_DMA_SendString(buf);
            while (UART1_DMA_IsBusy()) {}
        }
    }
    UART1_DMA_SendString("\r\n");
    while (UART1_DMA_IsBusy()) {}

    /* ---- 启动 DAC1 DMA 输出 ---- */
    if (DAC_SAW_Start() != 0)
    {
        UART1_DMA_SendString("DAC DMA start FAILED!\r\n");
        while (UART1_DMA_IsBusy()) {}
        Error_Handler();
    }

    /* DAC DMA 就绪后, 启动 TIM7 触发 */
    TIM7_Start();

    UART1_DMA_SendString("DAC1 DMA started! PA4 output sawtooth.\r\n");
    while (UART1_DMA_IsBusy()) {}

    while (1)
    {
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
