/**
 ****************************************************************************************************
 * @file        adc.h
 * @brief       ADC 驱动代码 (TIM2 定时触发 + DMA)
 *
 *              光敏电阻分压 → PC1 (ADC1_IN11)
 *              TIM2 TRGO 每 100ms 触发一次 ADC 转换
 *              DMA 循环传输转换结果
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
#define ADC_PHOTO_GPIO_CLK_ENABLE() __HAL_RCC_GPIOC_CLK_ENABLE()
#define ADC_PHOTO_ADC               ADC1
#define ADC_PHOTO_CHANNEL           ADC_CHANNEL_11

/* ---- DMA 缓冲区 ---- */
extern volatile uint16_t g_adc_dma_buf;      /* DMA 循环缓冲区 (1个元素) */
extern volatile uint8_t  g_adc_new_data;     /* 新数据标志: 1=有新数据 */

void     adc_init(void);                     /* ADC 初始化 (定时触发+DMA) */
void     adc_start_dma(void);                /* 启动 ADC DMA 传输 */
float    adc_get_voltage_v(uint16_t adc_val);  /* ADC 值 → 电压 (V) */

#endif /* __ADC_H */
