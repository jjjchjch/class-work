/**
 ****************************************************************************************************
 * @file        adc.h
 * @brief       ADC 驱动代码 (软件触发)
 *
 *              芯片内部温度 → ADC1_IN16 (ADC_CHANNEL_TEMPSENSOR)
 *              光敏电阻分压  → PC1  (ADC1_IN11)
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB2 = 16MHz, ADC CLK = APB2/4 = 4MHz
 ****************************************************************************************************
 */

#ifndef __ADC_H
#define __ADC_H

#include "stm32f4xx_hal.h"

/* ---- 芯片内部温度传感器 ---- */
#define ADC_TEMP_ADC                ADC1
#define ADC_TEMP_CHANNEL            ADC_CHANNEL_TEMPSENSOR   /* ADC1_IN16, 内部通道, 无需 GPIO */

/* ---- 光敏电阻分压 (PC1) ---- */
#define ADC_PHOTO_GPIO_PORT         GPIOC
#define ADC_PHOTO_GPIO_PIN          GPIO_PIN_1
#define ADC_PHOTO_GPIO_CLK_ENABLE() __HAL_RCC_GPIOC_CLK_ENABLE()
#define ADC_PHOTO_ADC               ADC1
#define ADC_PHOTO_CHANNEL           ADC_CHANNEL_11
#define ADC_PHOTO_CLK_ENABLE()      __HAL_RCC_ADC1_CLK_ENABLE()

void     adc_init(void);                                              /* ADC 初始化 */
uint32_t adc_get_result(uint32_t ch);                                 /* 获取单通道转换值 */
uint32_t adc_get_result_average(uint32_t ch, uint8_t times);          /* 获取通道多次平均转换值 */
float    adc_get_voltage(uint32_t ch, uint8_t times);                 /* 获取通道电压值 (V) */
uint32_t adc_get_voltage_mv(uint32_t ch, uint8_t times);              /* 获取通道电压值 (mV, 整数) */
float    adc_get_temperature(uint8_t times);                          /* 获取芯片内部温度 (°C) */

#endif /* __ADC_H */
