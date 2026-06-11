/**
 ****************************************************************************************************
 * @file        dac.h
 * @brief       DAC 波形发生器驱动 (TIM6 定时触发 + PA5 输出)
 *
 *              参考: 正点原子 实验9_2 定时触发DAC实验
 *              TIM6 TRGO 触发 DAC CH2 转换
 *              在 TIM6 中断回调中更新 DAC 值
 *              PA5 (DAC_OUT2) 输出波形
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 *              APB1 = 16MHz, TIM6 CLK = 16MHz
 ****************************************************************************************************
 */

#ifndef __DAC_H
#define __DAC_H

#include "stm32f4xx_hal.h"

/* ---- 波形类型枚举 ---- */
typedef enum {
    WAVE_NOISE   = 0,   /* 噪声波 */
    WAVE_TRIANGLE = 1   /* 三角波 */
} WaveType_t;

/* ---- 三角波查表参数 ---- */
#define DAC_TRI_STEPS    256U    /* 三角波步数 */
#define DAC_TRI_MAX      4095U   /* 12位 DAC 最大值 */

/* ---- TIM6 定时参数 (16MHz 系统时钟) ---- */
/*
 * PSC = 15   → 计数时钟 = 16MHz / 16 = 1MHz (1us/tick)
 * ARR = 39   → 溢出周期 = (39+1) × 1us = 40us
 * DAC 更新率 = 25kHz (适合波形输出, DAC建立时间约3~8us)
 */
#define DAC_TIM6_PSC     15U
#define DAC_TIM6_ARR     39U

/* ---- 导出函数 ---- */
void DAC_WaveGen_Init(void);                     /* 波形发生器初始化 (TIM6 + DAC CH2) */
void DAC_WaveGen_SetType(WaveType_t type);       /* 切换波形类型 */
WaveType_t DAC_WaveGen_GetType(void);            /* 获取当前波形类型 */

/* ---- TIM6 中断回调 (由 stm32f4xx_it.c 中的 TIM6_DAC_IRQHandler 调用) ---- */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif /* __DAC_H */
