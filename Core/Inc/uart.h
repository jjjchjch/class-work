#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

void UART1_Init(void);
void UART1_SendString(const char *text);
bool UART1_ReceiveByte(uint8_t *byte);
void UART1_SendADC(uint16_t adc_val);        /* 发送 ADC 值供串口示波器显示 */

#endif
