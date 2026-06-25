/**
 ****************************************************************************************************
 * @file        esp8266.h
 * @brief       ESP8266 AT 指令驱动 (USART3 + DMA + IDLE 中断)
 *
 *              从魔女科技 "进阶与提高4 ESP8266_DMA方式" 例程移植
 *              默认使用 STA 模式 (AT+CWMODE=1)
 *
 *              引脚分配:
 *                USART3_TX -> PB10   (ESP8266 RX)
 *                USART3_RX -> PB11   (ESP8266 TX)
 *                DMA1 Stream3 CH4 (TX), DMA1 Stream1 CH4 (RX)
 ****************************************************************************************************
 */
#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * 平台依赖 (uart.c 提供)
 * ================================================================ */
#define ESP8266_RX_BUF_SIZE   784     /* 接收缓冲区大小 */

/* ================================================================
 * AT 指令基础接口
 * ================================================================ */

/**
 * @brief  发送 AT 指令并等待期望应答
 * @param  cmdString    : AT 指令字符串 (需自带 \r\n)
 * @param  answerString : 期望返回的子串 (如 "OK", "WIFI GOT IP")
 * @param  waitTimesMS  : 超时时间 (ms, 实际约为 waitTimesMS*20ms 的循环粒度)
 * @retval 1 成功收到期望应答, 0 超时
 */
uint8_t ESP8266_SendAT(char *cmdString, char *answerString, uint32_t waitTimesMS);

/**
 * @brief  ESP8266 初始化 (复位 + 测试 AT + 设置 STA 模式)
 * @retval 1 初始化成功, 0 失败
 */
uint8_t ESP8266_Init(void);

/**
 * @brief  加入指定 AP (STA 模式)
 * @param  SSID     : WiFi 名称
 * @param  passWord : WiFi 密码
 * @param  timeout  : 超时 (循环次数, 每次 ~20ms)
 * @retval 1 连接成功, 0 失败
 */
uint8_t ESP8266_JoinAP(char *SSID, char *passWord, uint32_t timeout);

/**
 * @brief  查询并打印当前 IP 信息 (AT+CIFSR)
 */
void ESP8266_PrintIP(void);

#ifdef __cplusplus
}
#endif

#endif /* __ESP8266_H */
