#ifndef __TIMER_H
#define __TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/**
 * TIM3 PWM 输出
 *   CH1 → PA6 (AF2)   示波器 CH1
 *   CH2 → PA7 (AF2)   示波器 CH2
 *
 * SYSCLK = 16MHz (HSI)
 *   PSC = 15  → 计数时钟 1MHz（1?s/tick）
 *   ARR = 999 → 周期 1ms（1kHz）
 *   CCR = 250 → 占空比 25%
 */
void TIM3_PWM_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __TIMER_H */
