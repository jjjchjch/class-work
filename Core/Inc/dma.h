/**
 ****************************************************************************************************
 * @file        dma.h
 * @brief       DMA Çý¶¯Ä£¿é (USART1 TX, ADC2)
 *              STM32F407VET6
 ****************************************************************************************************
 */

#ifndef __DMA_H
#define __DMA_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * USART1 TX DMA (DMA2 Stream7 Channel4)
 * ================================================================ */
void DMA_USART1_TX_Init(UART_HandleTypeDef *huart);
bool DMA_USART1_TX_Send(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len);
bool DMA_USART1_TX_SendString(UART_HandleTypeDef *huart, const char *text);
bool DMA_USART1_TX_IsBusy(void);

/* ================================================================
 * ADC2 DMA (DMA2 Stream2 Channel1) ¡ª Ô¤Áô
 * ================================================================ */
void DMA_ADC2_Init(ADC_HandleTypeDef *hadc, uint32_t *buf, uint32_t len);
void DMA_ADC2_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_H */
