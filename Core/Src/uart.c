#include "uart.h"

#include "main.h"
#include <string.h>

static UART_HandleTypeDef huart1;

void UART1_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

void UART1_SendString(const char *text)
{
  size_t len = strlen(text);
  if (len == 0U)
  {
    return;
  }

  if (HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)len, 100U) != HAL_OK)
  {
    Error_Handler();
  }
}

bool UART1_ReceiveByte(uint8_t *byte)
{
  if (byte == NULL)
  {
    return false;
  }

  return (HAL_UART_Receive(&huart1, byte, 1U, 0U) == HAL_OK);
}
