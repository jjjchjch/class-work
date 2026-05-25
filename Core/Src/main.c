#include "main.h"
#include "gpio.h"
#include <stdint.h>
#include <string.h>
#include "uart.h"

static void SystemClock_Config(void);
static uint8_t ProcessCommand(const char *rx_buf);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  UART1_Init();

  char rx_buf[16] = {0};
  uint8_t rx_idx = 0;
  uint8_t ch = 0;

  while (1)
  {
    if (UART1_ReceiveByte(&ch))
    {
      if (rx_idx < (sizeof(rx_buf) - 1U))
      {
        rx_buf[rx_idx++] = (char)ch;
        rx_buf[rx_idx] = '\0';

        if (ProcessCommand(rx_buf) != 0U)
        {
          rx_idx = 0;
          rx_buf[0] = '\0';
        }
      }
    }
  }
}

static uint8_t ProcessCommand(const char *rx_buf)
{
  if (strcmp(rx_buf, "LED1 ON") == 0)
  {
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);
    return 1;
  }

  if (strcmp(rx_buf, "LED1 OFF") == 0)
  {
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_SET);
    return 1;
  }

  return 0;
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
