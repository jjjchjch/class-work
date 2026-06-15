#include "uart.h"

#include "main.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef huart1;
static DMA_HandleTypeDef  hdma_usart1_tx;
static volatile bool      dma_tx_busy = false;

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

/* ================================================================
 * DMA 发送驱动 (DMA2 Stream7 Channel4 → USART1_TX)
 *
 * STM32F407 DMA 映射:
 *   USART1_TX → DMA2, Stream 7, Channel 4
 *   USART1_RX → DMA2, Stream 5, Channel 4
 * ================================================================ */

/**
 * @brief  初始化 UART1 DMA 发送通道
 * @note   必须在 UART1_Init() 之后调用
 *         DMA2 Stream7, Channel4, 存储器→外设, 字节对齐
 */
void UART1_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    hdma_usart1_tx.Instance                 = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode                = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    hdma_usart1_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
        Error_Handler();
    }

    /* 将 DMA 句柄链接到 UART 句柄 */
    __HAL_LINKDMA(&huart1, hdmatx, hdma_usart1_tx);

    /* 使能 DMA2 Stream7 中断 (NVIC) */
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    /* 使能 USART1 中断 (NVIC) — HAL_UART_Transmit_DMA 依赖 TC 中断 */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/**
 * @brief  通过 DMA 发送字符串 (非阻塞)
 * @param  text: 要发送的字符串 (以 '\\0' 结尾)
 * @retval true:  启动发送成功
 * @retval false: DMA 正忙或参数无效
 */
bool UART1_DMA_SendString(const char *text)
{
    if (text == NULL || dma_tx_busy)
    {
        return false;
    }

    size_t len = strlen(text);
    if (len == 0U)
    {
        return false;
    }

    return UART1_DMA_SendData((const uint8_t *)text, (uint16_t)len);
}

/**
 * @brief  通过 DMA 发送原始数据 (非阻塞)
 * @param  data: 数据缓冲区指针
 * @param  len:  发送字节数
 * @retval true:  启动发送成功
 * @retval false: DMA 正忙或参数无效
 */
bool UART1_DMA_SendData(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U || dma_tx_busy)
    {
        return false;
    }

    dma_tx_busy = true;

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart1, (uint8_t *)data, len);
    if (status != HAL_OK)
    {
        dma_tx_busy = false;
        return false;
    }

    return true;
}

/**
 * @brief  查询 DMA 发送是否正在进行
 * @retval true:  DMA 正在发送
 * @retval false: DMA 空闲
 */
bool UART1_DMA_IsBusy(void)
{
    return dma_tx_busy;
}

/* ================================================================
 * DMA 中断服务 & 回调
 * ================================================================ */

/**
 * @brief  UART DMA 发送完成回调 (由 HAL 库在 DMA 传输完成中断中自动调用)
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        dma_tx_busy = false;
    }
}

/**
 * @brief  DMA2 Stream7 全局中断处理函数
 *         (对应 USART1_TX DMA 通道)
 */
void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

/**
 * @brief  USART1 全局中断处理函数
 *         HAL_UART_Transmit_DMA 在 DMA 搬运完成后依赖
 *         UART TC (传输完成) 中断来调用 TxCpltCallback
 */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}
