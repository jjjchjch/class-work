/**
 ****************************************************************************************************
 * @file        esp8266.c
 * @brief       ESP8266 AT 指令驱动实现 (USART3 + DMA + IDLE)
 *
 *              从魔女科技 "进阶与提高4 ESP8266_DMA方式" 例程移植并简化为 STA 模式
 ****************************************************************************************************
 */
#include "esp8266.h"
#include "uart.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>

/* ================================================================
 * 发送 AT 指令并等待期望应答
 * 调试模式: 把 "发送的命令" 和 "收到的回复" 全部打印到 USART1
 * ================================================================ */
uint8_t ESP8266_SendAT(char *cmdString, char *answerString, uint32_t waitTimesMS)
{
    uint32_t wait = waitTimesMS;

    /* 清空接收缓冲和计数 */
    Rx3Counter = 0;
    memset(U3_RxBuff, 0, ESP8266_RX_BUF_SIZE);

    /* 打印将要发送的命令到 USART1 */
    UART1_SendString("\r\n[TX] ");
    UART1_SendString(cmdString);

    /* 通过 USART3 发送 AT 指令给 ESP8266 */
    UART3_SendString(cmdString);

    /* 轮询等待期望应答 */
    while (wait--)
    {
        if (Rx3Counter)
        {
            if (strstr((char *)U3_RxBuff, answerString) != NULL)
            {
                /* 收到期望应答, 把完整回复打印到 USART1 */
                UART1_SendString("[RX OK] ");
                UART1_SendString((char *)U3_RxBuff);
                return 1;
            }
        }
        delay_ms(20);
    }

    /* 超时: 把收到的内容(可能为空)打印到 USART1, 方便排查 */
    UART1_SendString("[RX TIMEOUT] ");
    if (Rx3Counter)
    {
        UART1_SendString((char *)U3_RxBuff);
    }
    else
    {
        UART1_SendString("(no response — 检查接线和波特率)\r\n");
    }
    return 0;
}

/* ================================================================
 * 初始化 (调试版): 只做两步, 每步回复都打印到 USART1
 *   Step1: AT            -> 期望 "OK"
 *   Step2: AT+CWMODE=1   -> 期望 "OK"  (STA 模式)
 * 第3步 AT+CWJAP 由 main 调用 ESP8266_JoinAP 完成
 * ================================================================ */
uint8_t ESP8266_Init(void)
{
    uint8_t ret = 1;

    UART1_SendString("\r\n======== ESP8266 Init ========\r\n");

    /* Step 1: AT 连通性测试 */
    UART1_SendString("[Step 1] AT\r\n");
    if (ESP8266_SendAT("AT\r\n", "OK", 50))
        UART1_SendString("-----> Step1 OK\r\n");
    else
    {
        UART1_SendString("-----> Step1 FAIL\r\n");
        ret = 0;
    }

    /* Step 2: 设置 STA 模式 */
    UART1_SendString("[Step 2] AT+CWMODE=1\r\n");
    if (ESP8266_SendAT("AT+CWMODE=1\r\n", "OK", 100))
        UART1_SendString("-----> Step2 OK\r\n");
    else
    {
        UART1_SendString("-----> Step2 FAIL\r\n");
        ret = 0;
    }

    return ret;
}

/* ================================================================
 * Step 3: 加入 AP (AT+CWJAP)
 *   AT+CWJAP="jch","jchzcm123"  -> 期望 "WIFI GOT IP" 或 "OK"
 * 回复同样通过 ESP8266_SendAT 打印到 USART1
 * ================================================================ */
uint8_t ESP8266_JoinAP(char *SSID, char *passWord, uint32_t timeout)
{
    char cmd[96];
    uint8_t ok = 0;

    /* 拼接 AT+CWJAP="SSID","PASS" */
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, passWord);

    UART1_SendString("\r\n[Step 3] AT+CWJAP\r\n");
    /* 先等 "WIFI GOT IP" 表示拿到 IP, 退而求其次等 "OK" */
    if (ESP8266_SendAT(cmd, "WIFI GOT IP", timeout))
    {
        UART1_SendString("-----> Step3 GOT IP\r\n");
        ok = 1;
    }
    else if (strstr((char *)U3_RxBuff, "OK") != NULL)
    {
        UART1_SendString("-----> Step3 OK (no GOT IP yet)\r\n");
        ok = 1;
    }
    else
    {
        UART1_SendString("-----> Step3 FAIL\r\n");
    }

    return ok;
}

/* ================================================================
 * 查询并打印 IP 信息 (AT+CIFSR)
 * ================================================================ */
void ESP8266_PrintIP(void)
{
    UART1_SendString("[ESP] Query IP (AT+CIFSR):\r\n");
    Rx3Counter = 0;
    memset(U3_RxBuff, 0, ESP8266_RX_BUF_SIZE);
    UART3_SendString("AT+CIFSR\r\n");
    delay_ms(500);
    if (Rx3Counter)
    {
        UART1_SendString((char *)U3_RxBuff);
    }
    else
    {
        UART1_SendString("(no response)\r\n");
    }
}
