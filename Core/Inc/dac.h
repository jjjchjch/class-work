/**
 ****************************************************************************************************
 * @file        dac.h
 * @brief       DAC 锯齿波输出驱动 (TIM5 定时触发 + DMA)
 *
 *              TIM5 TRGO 每 1ms 触发一次 DAC CH1 转换
 *              DMA1_Stream5_Channel7 循环传输锯齿波表到 DAC_DHR12R1
 *              PA4 (DAC_OUT1) 输出 0V → 3.3V 锯齿波
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB1 = 16MHz, TIM5 CLK = 16MHz
 ****************************************************************************************************
 */

#ifndef __DAC_H
#define __DAC_H

#include "stm32f4xx_hal.h"

/* ---- 锯齿波参数 ---- */
#define DAC_RAMP_STEPS    256U    /* 锯齿波步数 (每步 1ms, 周期 256ms) */
#define DAC_RAMP_MAX      4095U   /* 12位 DAC 最大值 */

/* ---- DMA 缓冲区 ---- */
extern volatile uint16_t g_dac_ramp_table[DAC_RAMP_STEPS];  /* 锯齿波查找表 */

void DAC_Sawtooth_Init(void);    /* DAC 锯齿波初始化 (TIM5触发 + DMA) */

#endif /* __DAC_H */
