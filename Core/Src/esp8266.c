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
 *   必须等 "OK"，不等 "WIFI GOT IP"
 *   WIFI GOT IP 后 ESP8266 可能还没完全空闲
 * ================================================================ */
uint8_t ESP8266_JoinAP(char *SSID, char *passWord, uint32_t timeout)
{
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, passWord);

    UART1_SendString("\r\n[Step 3] AT+CWJAP\r\n");
    if (ESP8266_SendAT(cmd, "OK", timeout))
    {
        UART1_SendString("-----> Step3 OK\r\n");
        /* 关键：等模块彻底空闲 */
        delay_ms(3000);
        return 1;
    }
    UART1_SendString("-----> Step3 FAIL\r\n");
    return 0;
}

/* ================================================================
 * 查询并打印 IP 信息 (AT+CIFSR)
 * ================================================================ */
void ESP8266_PrintIP(void)
{
    UART1_SendString("[ESP] Query IP (AT+CIFSR):\r\n");
    if (ESP8266_SendAT("AT+CIFSR\r\n", "OK", 200))
    {
        UART1_SendString("[ESP] CIFSR OK\r\n");
    }
    else
    {
        UART1_SendString("[ESP] CIFSR FAIL\r\n");
    }
    delay_ms(1000);
}

/* ================================================================
 * 启动 TCP 透传模式
 * 顺序: 清理残留 → CIPMUX=0 → CIPSTART → CIPMODE=1 → CIPSEND
 * ================================================================ */
uint8_t ESP8266_StartTransparent(char *serverIP, uint16_t port)
{
    char cmd[96];
    uint8_t i;

    UART1_SendString("\r\n======== Start Transparent ========\r\n");

    /* 先确认模块空闲 */
    UART1_SendString("[Prepare] Check ESP8266 idle\r\n");
    ESP8266_SendAT("AT\r\n", "OK", 100);
    delay_ms(500);

    /* 退出可能残留的透传模式/连接状态 */
    UART1_SendString("[Prepare] Set normal mode\r\n");
    ESP8266_SendAT("AT+CIPMODE=0\r\n", "OK", 100);
    delay_ms(300);
    UART1_SendString("[Prepare] Close old TCP link\r\n");
    ESP8266_SendAT("AT+CIPCLOSE\r\n", "OK", 50);
    delay_ms(1000);

    /* Step 4: 单连接模式 */
    UART1_SendString("[Step 4] AT+CIPMUX=0\r\n");
    if (!ESP8266_SendAT("AT+CIPMUX=0\r\n", "OK", 100))
    {
        UART1_SendString("-----> Step4 FAIL\r\n");
        return 0;
    }
    UART1_SendString("-----> Step4 OK\r\n");
    delay_ms(500);

    /* Step 5: 建立 TCP 连接 (最多重试 3 次) */
    UART1_SendString("[Step 5] AT+CIPSTART\r\n");
    snprintf(cmd, sizeof(cmd),
             "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",
             serverIP,
             port);
    for (i = 0; i < 3; i++)
    {
        UART1_SendString("[TCP Try] ");
        UART1_SendString(cmd);
        if (ESP8266_SendAT(cmd, "CONNECT", 500))
        {
            UART1_SendString("-----> Step5 CONNECT OK\r\n");
            break;
        }
        /* 有些固件返回 ALREADY CONNECT，也算连接存在 */
        if (strstr((char *)U3_RxBuff, "ALREADY CONNECT") != NULL)
        {
            UART1_SendString("-----> Step5 ALREADY CONNECT\r\n");
            break;
        }
        UART1_SendString("-----> Step5 retry...\r\n");
        ESP8266_SendAT("AT+CIPCLOSE\r\n", "OK", 50);
        delay_ms(1500);
    }
    if (i >= 3)
    {
        UART1_SendString("-----> Step5 FAIL\r\n");
        UART1_SendString("[Debug] Please check TCP Server, IP, firewall\r\n");
        return 0;
    }
    delay_ms(1000);

    /* Step 6: 开启透传模式 */
    UART1_SendString("[Step 6] AT+CIPMODE=1\r\n");
    if (!ESP8266_SendAT("AT+CIPMODE=1\r\n", "OK", 100))
    {
        UART1_SendString("-----> Step6 FAIL\r\n");
        return 0;
    }
    UART1_SendString("-----> Step6 OK\r\n");
    delay_ms(500);

    /* Step 7: 进入透传发送 */
    UART1_SendString("[Step 7] AT+CIPSEND\r\n");
    if (!ESP8266_SendAT("AT+CIPSEND\r\n", ">", 200))
    {
        UART1_SendString("-----> Step7 FAIL\r\n");
        return 0;
    }
    UART1_SendString("-----> Step7 OK\r\n");

    UART1_SendString("======== Transparent Mode Active ========\r\n");
    return 1;
}
