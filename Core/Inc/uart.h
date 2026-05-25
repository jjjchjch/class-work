#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

void UART1_Init(void);
void UART1_SendString(const char *text);
bool UART1_ReceiveByte(uint8_t *byte);

#endif
