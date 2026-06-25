#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* ================================================================
 * USART1 (PA9/PA10) — 调试串口
 * ================================================================ */
void UART1_Init(void);
void UART1_SendString(const char *text);
bool UART1_ReceiveByte(uint8_t *byte);
void UART1_SendADC(uint16_t adc_val);        /* 发送 ADC 值供串口示波器显示 */

/* ---- DMA 发送 (DMA2 Stream7 CH4 → USART1_TX) ---- */
void UART1_DMA_Init(void);
bool UART1_DMA_SendString(const char *text);
bool UART1_DMA_SendData(const uint8_t *data, uint16_t len);
bool UART1_DMA_IsBusy(void);

/* ================================================================
 * USART3 (PB10 TX / PB11 RX) — ESP8266 通信
 *               DMA1 Stream3 CH4 (TX), DMA1 Stream1 CH4 (RX)
 *               接收使用 DMA + IDLE 空闲中断 (HAL_UARTEx_ReceiveToIdle_DMA)
 * ================================================================ */
#define U3_TX_SIZE   512
#define U3_RX_SIZE   784          /* ESP8266 AT 单帧最长 ~1056 字节, 784 已足够 */

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef  hdma_usart3_tx;
extern DMA_HandleTypeDef  hdma_usart3_rx;

extern uint8_t  U3_TxBuff[U3_TX_SIZE];
extern uint8_t  U3_RxBuff[U3_RX_SIZE];
extern volatile uint16_t Rx3Counter;        /* ESP8266 一帧接收到的字节数 */

void UART3_Init(void);                      /* 初始化 USART3 + DMA + IDLE, 并启动 RX */
void UART3_SendString(const char *text);    /* 阻塞式发送字符串 */
void UART3_SendData(const uint8_t *data, uint16_t len);

#endif
