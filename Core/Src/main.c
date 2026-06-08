/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       ADC 软件触发实验 (参考: 实验7 ADC实验)
 *
 *              芯片内部温度 → ADC1_IN16 (内部温度传感器)
 *              光敏电阻分压  → PC1       (ADC1_IN11)
 *              LCD           → ILI9341   (FSMC 接口)
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 ****************************************************************************************************
 */

#include "main.h"
#include "gpio.h"
#include "adc.h"
#include "bsp_LCD_ILI9341.h"
#include <stdio.h>

static void SystemClock_Config(void);

int main(void)
{
    uint16_t adcx;
    float    temp;
    static char strTemp[30];

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* ---- LCD 初始化 ---- */
    LCD_Init();
    LCD_SetDir(0);
    LCD_Fill(0, 0, 240, 320, BLACK);

    /* ---- 标题 ---- */
    LCD_Fill(0, 0, 240, 40, DARKBLUE);
    LCD_String(40, 8, "ADC Experiment", 16, WHITE, DARKBLUE);
    LCD_Line(0, 40, 240, 40, CYAN);

    /* ---- 芯片内部温度 ADC1_IN16 ---- */
    LCD_String(5,  48,  "Chip Temp IN16", 24, BLACK, GREEN);
    LCD_String(5,  78,  "ADC:", 24, CYAN, BLACK);
    LCD_String(5,  108, "V  :", 24, CYAN, BLACK);
    LCD_String(5,  138, "T  :", 24, CYAN, BLACK);

    /* ---- 分隔线 ---- */
    LCD_Line(5, 170, 235, 170, GRAY);

    /* ---- 光敏电阻 PC1 ---- */
    LCD_String(5,  178, "Photoresist  PC1", 24, BLACK, GREEN);
    LCD_String(5,  208, "ADC:", 24, CYAN, BLACK);
    LCD_String(5,  238, "V  :", 24, CYAN, BLACK);

    /* ---- 底部 ---- */
    LCD_Line(0, 288, 240, 288, CYAN);
    LCD_String(60, 295, "jinchenghao", 24, YELLOW, BLACK);

    /* ---- ADC 初始化 ---- */
    adc_init();

    while (1)
    {
        /* ======== 芯片内部温度 ADC1_IN16 ======== */
        adcx = (uint16_t)adc_get_result_average(ADC_TEMP_CHANNEL, 10);
        temp = (float)adcx * (3.3f / 4096.0f);

        sprintf(strTemp, "%4d    ", adcx);
        LCD_String(80, 78, strTemp, 24, GREEN, BLACK);

        sprintf(strTemp, "%1.2f V    ", (double)temp);
        LCD_String(80, 108, strTemp, 24, GREEN, BLACK);

        /* 温度转换: T = (V25 - V_sense) / 0.0025 + 25 */
        {
            float temp_c = adc_get_temperature(10);
            sprintf(strTemp, "%2.1f C    ", (double)temp_c);
            LCD_String(80, 138, strTemp, 24, GREEN, BLACK);
        }

        /* ======== 光敏电阻 PC1 ======== */
        adcx = (uint16_t)adc_get_result_average(ADC_PHOTO_CHANNEL, 10);
        temp = (float)adcx * (3.3f / 4096.0f);

        sprintf(strTemp, "%4d    ", adcx);
        LCD_String(80, 208, strTemp, 24, GREEN, BLACK);

        sprintf(strTemp, "%1.2f V    ", (double)temp);
        LCD_String(80, 238, strTemp, 24, GREEN, BLACK);

        HAL_Delay(200);
    }
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
