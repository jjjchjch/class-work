/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       ESP8266 WiFi (STA 模式) 连接测试
 *              USART1 (PA9/PA10)   调试输出 (115200)
 *              USART3 (PB10/PB11)  ESP8266 通信
 ****************************************************************************************************
 */

#include "main.h"
#include "uart.h"
#include "delay.h"
#include "bsp_LCD_ILI9341.h"
#include "esp8266.h"
#include <stdio.h>
#include <string.h>

/* ---- 颜色 ---- */
#define BLACK   0x0000
#define WHITE   0xFFFF
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x7FFF
#define YELLOW  0xFFE0

/* ---- ESP8266 WiFi 配置 (STA 模式) ---- */
#define WIFI_SSID     "jch"
#define WIFI_PASSWORD "jchzcm123"

/*===========================================================================
 * 系统时钟: HSI 16MHz
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
 * 主程序: 仅 ESP8266 WiFi 连接测试
 *         初始化 → AT → CWMODE=1 → CWJAP → CIFSR → 主循环 LED 心跳
 *===========================================================================*/
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    /* ---- UART1 (调试输出 115200) ---- */
    UART1_Init();
    UART1_SendString("\r\n========== ESP8266 WiFi Test ==========\r\n");

    /* ---- LED (PC5) 心跳 ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* ---- LCD 状态显示 ---- */
    LCD_Init();
    LCD_SetDir(0);
    LCD_Fill(0, 0, 240, 320, BLACK);
    LCD_Fill(0, 0, 239, 24, BLUE);
    LCD_String(30, 3, (char *)"ESP8266 WiFi Test", 16, WHITE, BLUE);

    /* ---- ESP8266 WiFi (USART3, STA 模式) ---- */
    UART3_Init();
    LCD_String(4, 40, (char *)"WiFi: Connecting...", 16, YELLOW, BLACK);

    if (ESP8266_Init())
    {
        UART1_SendString("[Main] ESP8266 Init OK, joining AP\r\n");
        if (ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD, 400))
        {
            UART1_SendString("[Main] JoinAP OK\r\n");
            LCD_Fill(0, 40, 239, 56, BLACK);
            LCD_String(4, 40, (char *)"WiFi: Connected", 16, GREEN, BLACK);
            ESP8266_PrintIP();
        }
        else
        {
            UART1_SendString("[Main] JoinAP FAIL\r\n");
            LCD_Fill(0, 40, 239, 56, BLACK);
            LCD_String(4, 40, (char *)"WiFi: JoinAP FAIL", 16, RED, BLACK);
        }
    }
    else
    {
        UART1_SendString("[Main] ESP8266 Init FAIL\r\n");
        LCD_Fill(0, 40, 239, 56, BLACK);
        LCD_String(4, 40, (char *)"WiFi: No ESP8266", 16, RED, BLACK);
    }

    /* ---- 主循环: 仅 LED 心跳, 不再有其他输出干扰 ---- */
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5);
        delay_ms(500);
    }
}

/* 桩函数 */
ADC_HandleTypeDef hadc1 = {0};
void ADC1_ConvCpltCallback(uint16_t val) { (void)val; }
void Error_Handler(void) { __disable_irq(); while (1) {} }

