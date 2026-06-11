/**
 ****************************************************************************************************
 * @file        adc.h
 * @brief       ADC 驱动 (单次转换 + 轮询, 最简可靠方案)
 *
 *              光敏电阻分压 → PC1 (ADC1_IN11)
 *              adc_read() 启动转换 → 等待完成 → 返回结果
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 ****************************************************************************************************
 */

#ifndef __ADC_H
#define __ADC_H

#include "stm32f4xx_hal.h"

/* ---- 光敏电阻分压 (PC1) ---- */
#define ADC_PHOTO_GPIO_PORT         GPIOC
#define ADC_PHOTO_GPIO_PIN          GPIO_PIN_1
#define ADC_PHOTO_ADC               ADC1
#define ADC_PHOTO_CHANNEL           ADC_CHANNEL_11

void     adc_init(void);                      /* ADC 初始化 */
uint16_t adc_read(void);                      /* 单次转换 + 返回 raw 值 (0~4095) */
float    adc_get_voltage_v(uint16_t adc_val); /* ADC 值 → 电压 (V) */

#endif /* __ADC_H */
