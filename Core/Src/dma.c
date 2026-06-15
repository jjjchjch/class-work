/**
 ****************************************************************************************************
 * @file        dma.c
 * @brief       DMA 驱动实现
 *
 *              USART1 TX → DMA2 Stream7 Channel4
 *              ADC2     → DMA2 Stream2 Channel1
 *
 *              STM32F407VET6
 ****************************************************************************************************
 */

#include "dma.h"
#include "main.h"

/* ================================================================
 * USART1 TX DMA (DMA2 Stream7 CH4)
 * ================================================================ */
static DMA_HandleTypeDef hdma_usart1_tx;
static volatile bool     dma_usart1_tx_busy = false;

/**
 * @brief  初始化 USART1 TX DMA 通道
 * @param  huart: UART 句柄指针 (由 uart.c 传入)
 */
void DMA_USART1_TX_Init(UART_HandleTypeDef *huart)
{
    if (huart == NULL) return;

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

    __HAL_LINKDMA(huart, hdmatx, hdma_usart1_tx);

    /* NVIC: DMA2 Stream7 */
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

/**
 * @brief  DMA 发送原始数据 (非阻塞)
 * @retval true 成功启动, false 忙或参数无效
 */
bool DMA_USART1_TX_Send(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len)
{
    if (huart == NULL || data == NULL || len == 0U || dma_usart1_tx_busy)
    {
        return false;
    }

    dma_usart1_tx_busy = true;

    if (HAL_UART_Transmit_DMA(huart, (uint8_t *)data, len) != HAL_OK)
    {
        dma_usart1_tx_busy = false;
        return false;
    }

    return true;
}

/**
 * @brief  DMA 发送字符串 (非阻塞)
 */
bool DMA_USART1_TX_SendString(UART_HandleTypeDef *huart, const char *text)
{
    if (huart == NULL || text == NULL || dma_usart1_tx_busy)
    {
        return false;
    }

    size_t len = 0;
    while (text[len] != '\0') len++;
    if (len == 0U) return false;

    return DMA_USART1_TX_Send(huart, (const uint8_t *)text, (uint16_t)len);
}

/**
 * @brief  查询 DMA 是否忙碌
 */
bool DMA_USART1_TX_IsBusy(void)
{
    return dma_usart1_tx_busy;
}

/* ================================================================
 * DMA 中断服务
 * ================================================================ */

/**
 * @brief  DMA2 Stream7 中断 (USART1 TX)
 */
void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

/* ================================================================
 * HAL 回调 (UART DMA 发送完成)
 * ================================================================ */

/**
 * @brief  UART DMA 发送完成回调
 *         由 USART1_IRQHandler → HAL_UART_IRQHandler 调用
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        dma_usart1_tx_busy = false;
    }
}

/* ================================================================
 * ADC2 DMA (DMA2 Stream2 CH1) — 预留接口
 * ================================================================ */
static DMA_HandleTypeDef hdma_adc2;

void DMA_ADC2_Init(ADC_HandleTypeDef *hadc, uint32_t *buf, uint32_t len)
{
    if (hadc == NULL || buf == NULL) return;

    __HAL_RCC_DMA2_CLK_ENABLE();

    hdma_adc2.Instance                 = DMA2_Stream2;
    hdma_adc2.Init.Channel             = DMA_CHANNEL_1;
    hdma_adc2.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc2.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc2.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc2.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc2.Init.Mode                = DMA_CIRCULAR;
    hdma_adc2.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_adc2.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_adc2) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc2);

    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
}

void DMA_ADC2_Start(void)
{
    /* 预留: HAL_ADC_Start_DMA(...) */
}

/**
 * @brief  DMA2 Stream2 中断 (ADC2)
 */
void DMA2_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc2);
}
