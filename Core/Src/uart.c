#include "uart.h"

#include "main.h"
#include <string.h>
#include <stdio.h>

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

/**
 * @brief  发送 ADC 值供串口示波器显示波形
 *         格式: "XXXX\r\n" (ADC 值 + 回车换行)
 *         兼容 VOFA+ (Justfire 协议)、SerialPlot、Arduino 串口绘图器等
 * @param  adc_val: 12位 ADC 值 (0~4095)
 */
void UART1_SendADC(uint16_t adc_val)
{
    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d\r\n", adc_val);
    if (len > 0)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, 10U);
    }
}
