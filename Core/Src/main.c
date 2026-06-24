/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       BMP280 气压温度采集 (移植自"进阶与提高2 BMP280实验")
 *
 *              硬件: STM32F407VET6 + ILI9341 LCD (240×320)
 *                    BMP280 接 PE4(SCL) / PE6(SDA)
 *                    UART1: PA9(TX), PA10(RX), 115200bps
 *
 *              功能:
 *              - 每 100ms 读取 BMP280 温度+气压
 *              - LCD 显示气压值、温度值
 *              - 串口 printf 输出温气压数据
 *              - LED1(PC5) 每 2 秒翻转一次
 ****************************************************************************************************
 */

#include "main.h"
#include "uart.h"
#include "delay.h"
#include "key.h"
#include "bsp_LCD_ILI9341.h"
#include "bsp_W25Q128.h"
#include "bmp280.h"
#include <stdio.h>

/* ---- LCD 颜色 ---- */
#define BLACK   0x0000
#define WHITE   0xFFFF
#define BLUE    0x001F

/* ---- 串口缓冲 ---- */
char g_uartBuf[128];

/*===========================================================================
 * 系统时钟: HSI 16MHz (内部 RC, 最可靠)
 *===========================================================================*/
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

/*===========================================================================
 * 主函数
 *===========================================================================*/
int main(void)
{
    uint16_t t = 0;
    char strTemp[30];

    HAL_Init();
    SystemClock_Config();

    /* ---- UART ---- */
    UART1_Init();
    UART1_SendString("\r\n=== BMP280 Demo ===\r\n");
    UART1_SendString("Format: Temp(C),Press(kPa)\r\n\r\n");

    /* ---- LED GPIO ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* ---- KEY ---- */
    key_init();

    /* ---- LCD ---- */
    LCD_Init();
    LCD_SetDir(0);
    LCD_Fill(0, 0, 240, 320, WHITE);

    /* ---- W25Q128 ---- */
    W25Q128_Init();

    /* ---- LCD 标题 ---- */
    LCD_String(20, 8, (char *)"BMP280气压温度采集", 24, WHITE, BLACK);
    LCD_String(5, 60, (char *)" 气压值:  ", 24, WHITE, BLACK);

    /* ---- BMP280 ---- */
    Bmp280Init();

    /* ---- 主循环 ---- */
    while (1)
    {
        if (t % 10 == 0)   /* 每 100ms 读取一次 */
        {
            bmp280_GetValue();

            /* 串口示波器: 双通道 T(C), P(kPa) */
            sprintf(g_uartBuf, "%.1f,%.3f\r\n",
                    (double)Bmp280Data.T, (double)(Bmp280Data.P / 1000.0));
            UART1_SendString(g_uartBuf);

            /* LCD 气压 */
            sprintf(strTemp, "%4.1f  ", (double)Bmp280Data.P);
            LCD_String(105, 60, strTemp, 20, WHITE, BLUE);

            /* LCD 温度 */
            sprintf(strTemp, "温度值：%4.1f  ", (double)Bmp280Data.T);
            LCD_String(10, 90, strTemp, 20, WHITE, BLUE);
        }

        delay_ms(10);
        t++;

        if (t == 20)   /* 每 2 秒翻转 LED */
        {
            t = 0;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5);
        }
    }
}

/*===========================================================================
 * printf 重定向到 USART1 (寄存器轮询)
 *===========================================================================*/
int __io_putchar(int ch)
{
    while ((USART1->SR & USART_SR_TXE) == 0) { }
    USART1->DR = (uint8_t)ch;
    return ch;
}

/* 桩函数 */
ADC_HandleTypeDef hadc1 = {0};
void ADC1_ConvCpltCallback(uint16_t val) { (void)val; }
void Error_Handler(void) { __disable_irq(); while (1) {} }

