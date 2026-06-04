#ifndef __TIMER_H
#define __TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* ---- TIM3 全局句柄（PWM + 输入捕获共用）---- */
extern TIM_HandleTypeDef htim3;

/**
 * TIM3 输入捕获结果结构体
 */
typedef struct {
    uint32_t period_us;       /* 周期 (us)           */
    uint32_t high_us;         /* 高电平时间 (us)      */
    uint32_t duty;            /* 占空比 (千分比, 0-1000) */
    uint32_t freq_hz;         /* 频率 (Hz)            */
    uint8_t  valid;           /* 数据有效标志          */
} TIM3_IC_Result_t;

extern volatile TIM3_IC_Result_t g_ic_result;

/**
 * TIM3 PWM 输出（APB1，16位定时器）
 *   CH1 → PA6 (AF2)   50% 占空比, 1kHz
 *
 * SYSCLK = 16MHz (HSI)
 *   PSC = 15  → 计数时钟 1MHz（1us/tick）
 *   ARR = 999 → 周期 1ms（1kHz）
 *   CCR1 = 500 → PA6 占空比 50%
 */
void TIM3_PWM_Init(void);

/**
 * TIM3 输入捕获初始化
 *   CH2 → PA7 (AF2)   捕获 PA6 输出的 PWM 波形
 *
 * 使用 TIM3 CH2 双沿捕获（上升沿 + 下降沿）
 * 测量周期、高电平时间、占空比
 */
void TIM3_IC_Init(void);

/**
 * TIM4 软件PWM呼吸灯
 *   PB1 (LED2, 低电平点亮)
 *   PWM频率 200Hz，2s 呼吸周期
 */
void TIM4_Breathe_Init(void);

/**
 * TIM2 数字钟定时器（APB1，32位定时器）
 *   1秒中断，用于更新数字钟
 *
 * SYSCLK = 16MHz (HSI)
 *   PSC = 15999 → 计数时钟 1kHz（1ms/tick）
 *   ARR = 999   → 1s 溢出周期
 */
void TIM2_Clock_Init(void);

/* 数字钟时间变量（TIM2 ISR 中更新） */
extern volatile uint8_t g_clock_hour;
extern volatile uint8_t g_clock_min;
extern volatile uint8_t g_clock_sec;
extern volatile uint8_t g_clock_update;   /* 1 = 需要刷新显示 */

#ifdef __cplusplus
}
#endif

#endif /* __TIMER_H */
