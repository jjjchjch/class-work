#include "uart.h"
#include "dma.h"

#include "main.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef huart1;

/* ================================================================
 * USART3 (PB10 TX / PB11 RX) - ESP8266
 * DMA1 Stream3 CH4 (TX), DMA1 Stream1 CH4 (RX), IDLE 空闲中断
 * ================================================================ */
UART_HandleTypeDef huart3;
DMA_HandleTypeDef  hdma_usart3_tx;
DMA_HandleTypeDef  hdma_usart3_rx;

uint8_t  U3_TxBuff[U3_TX_SIZE];
uint8_t  U3_RxBuff[U3_RX_SIZE];
volatile uint16_t Rx3Counter = 0;

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
 * DMA 发送 (委托给 dma.c)
 * ================================================================ */

void UART1_DMA_Init(void)
{
    DMA_USART1_TX_Init(&huart1);

    /* USART1 NVIC (HAL_UART_Transmit_DMA 依赖 TC 中断) */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

bool UART1_DMA_SendString(const char *text)
{
    return DMA_USART1_TX_SendString(&huart1, text);
}

bool UART1_DMA_SendData(const uint8_t *data, uint16_t len)
{
    return DMA_USART1_TX_Send(&huart1, data, len);
}

bool UART1_DMA_IsBusy(void)
{
    return DMA_USART1_TX_IsBusy();
}

/**
 * @brief  USART1 中断处理
 *         HAL_UART_Transmit_DMA 依赖 UART TC 中断
 */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

/* ================================================================
 * USART3 - ESP8266 通信 (DMA + IDLE)
 * ================================================================ */

/**
 * @brief  初始化 USART3: PB10(TX) / PB11(RX), 115200, 8N1
 *         同时配置 DMA1 Stream3(TX) / Stream1(RX), 并启动 IDLE 接收
 */
void UART3_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* PB10 (TX) / PB11 (RX) 复用 USART3 */
    GPIO_InitStruct.Pin       = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart3.Instance          = USART3;
    huart3.Init.BaudRate     = 115200;
    huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    huart3.Init.StopBits     = UART_STOPBITS_1;
    huart3.Init.Parity       = UART_PARITY_NONE;
    huart3.Init.Mode         = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        Error_Handler();
    }

    /* TX DMA: DMA1 Stream3 Channel4 */
    hdma_usart3_tx.Instance                 = DMA1_Stream3;
    hdma_usart3_tx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart3_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart3_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart3_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart3_tx.Init.Mode                = DMA_NORMAL;
    hdma_usart3_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    hdma_usart3_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_tx) != HAL_OK) Error_Handler();
    __HAL_LINKDMA(&huart3, hdmatx, hdma_usart3_tx);

    /* RX DMA: DMA1 Stream1 Channel4 */
    hdma_usart3_rx.Instance                 = DMA1_Stream1;
    hdma_usart3_rx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart3_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart3_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart3_rx.Init.Mode                = DMA_NORMAL;
    hdma_usart3_rx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    hdma_usart3_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK) Error_Handler();
    __HAL_LINKDMA(&huart3, hdmarx, hdma_usart3_rx);

    /* NVIC: USART3 + DMA TX + DMA RX */
    HAL_NVIC_SetPriority(USART3_IRQn,           1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn,     1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn,     1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

    /* 启动 IDLE 接收 (DMA 收满一帧或总线空闲时触发回调) */
    Rx3Counter = 0;
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, U3_RxBuff, U3_RX_SIZE);
}

/**
 * @brief  USART3 阻塞式发送字符串
 */
void UART3_SendString(const char *text)
{
    if (text == NULL) return;
    size_t len = strlen(text);
    if (len == 0U) return;
    HAL_UART_Transmit(&huart3, (uint8_t *)text, (uint16_t)len, 500U);
}

/**
 * @brief  USART3 阻塞式发送原始数据
 */
void UART3_SendData(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U) return;
    HAL_UART_Transmit(&huart3, (uint8_t *)data, len, 500U);
}

/**
 * @brief  USART3 全局中断 — HAL 处理 IDLE/TC/错误
 */
void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3);
}

/**
 * @brief  DMA1 Stream3 中断 (USART3 TX)
 */
void DMA1_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_tx);
}

/**
 * @brief  DMA1 Stream1 中断 (USART3 RX)
 */
void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_rx);
}

/**
 * @brief  HAL 接收事件回调 (DMA 完成或 IDLE 触发)
 *         在此处记录本次接收字节数, 并重启下一轮 DMA 接收
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART3)
    {
        /* 仅在没处理完上一帧时才覆盖, 避免 ESP8266_SendAT 漏读 */
        if (Rx3Counter == 0)
        {
            Rx3Counter = Size;
        }
        /* 重启下一轮 DMA 接收 */
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, U3_RxBuff, U3_RX_SIZE);
    }
}
