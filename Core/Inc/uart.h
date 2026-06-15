#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

/* ---- 阻塞式 UART ---- */
void UART1_Init(void);
void UART1_SendString(const char *text);
bool UART1_ReceiveByte(uint8_t *byte);
void UART1_SendADC(uint16_t adc_val);        /* 发送 ADC 值供串口示波器显示 */

/* ---- DMA 发送 (DMA2 Stream7 CH4 → USART1_TX) ---- */
void UART1_DMA_Init(void);
bool UART1_DMA_SendString(const char *text);
bool UART1_DMA_SendData(const uint8_t *data, uint16_t len);
bool UART1_DMA_IsBusy(void);

#endif
