/**
 ****************************************************************************************************
 * @file        dac.h
 * @brief       DAC 驱动 (参考: 正点原子 实验9_2 定时触发DAC实验)
 *
 *              PA4 (DAC_OUT1): ADC→DAC 光敏电压跟随输出 (软件触发)
 *              PA5 (DAC_OUT2): 波形发生器 (TIM6硬件触发, 噪声/三角波)
 *
 *              STM32F407VET6, SYSCLK = 16MHz (HSI)
 ****************************************************************************************************
 */

#ifndef __DAC_H
#define __DAC_H

#include "stm32f4xx_hal.h"

/* ---- 波形类型枚举 ---- */
typedef enum {
    WAVE_NOISE    = 0,   /* 噪声波 */
    WAVE_TRIANGLE = 1    /* 三角波 */
} WaveType_t;

/* ---- 三角波查表参数 ---- */
#define DAC_TRI_STEPS    256U
#define DAC_TRI_MAX      4095U

/* ---- TIM6 定时参数 (16MHz) ---- */
#define DAC_TIM6_PSC     15U     /* 16MHz/16 = 1MHz */
#define DAC_TIM6_ARR     39U     /* 1MHz/40  = 25kHz */

/* ==================== DAC 基本驱动 (参考正点原子) ==================== */

void DAC_Init(void);                                  /* 初始化 DAC (PA4 CH1 + PA5 CH2) */
void DAC_SetCh1(uint16_t val);                        /* 设置 PA4 电压 (0~4095) */
void DAC_SetCh2(uint16_t val);                        /* 设置 PA5 电压 (0~4095) */

/* ==================== 波形发生器 ==================== */

void DAC_WaveGen_Start(void);                         /* 启动 TIM6 波形发生器 */
void DAC_WaveGen_SetType(WaveType_t type);            /* 切换波形类型 */
WaveType_t DAC_WaveGen_GetType(void);                 /* 获取当前波形类型 */

/* ---- TIM6 中断回调 ---- */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif /* __DAC_H */
