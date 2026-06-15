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
 * TIM2 定时触发初始化 (100ms TRGO, 用于触发 ADC)
 *
 * SYSCLK = 16MHz (HSI)
 *   PSC = 15999 → 计数时钟 1kHz (1ms/tick)
 *   ARR = 99    → 溢出周期 100ms
 *   MMS = 010   → 更新事件作为 TRGO 输出
 *
 * ADC 配置为 TIM2 TRGO 触发, 每 100ms 自动转换一次
 */
void TIM2_TRGO_Init(void);

/**
 * TIM3 TRGO 初始化 (10ms, 触发 ADC2)
 *
 * SYSCLK = 16MHz (HSI)
 *   PSC = 15999 → 1kHz
 *   ARR = 9     → 10ms
 *   MMS = 010   → TRGO = 更新事件
 *
 * STM32F407 ADC EXTSEL=8 → TIM3_TRGO
 */
void TIM3_ADC_Trigger_Init(void);

/**
 * TIM4 TRGO 初始化 (10ms, 触发 ADC2) — 不可用!
 * STM32F407 ADC 不支持 TIM4_TRGO 作为外部触发源.
 * 保留仅作为普通定时器参考.
 */
void TIM4_ADC_Trigger_Init(void);

/**
 * TIM3 PWM 波形输出 (PA6)
 *
 * 将 ADC 采样值通过 PWM 占空比输出到 PA6
 * 外接 RC 低通滤波即可得到模拟电压波形 (PWM DAC)
 *
 *   PSC = 15  → 计数时钟 1MHz
 *   ARR = 999 → 周期 1ms (1kHz)
 *   CCR1 = adc_val * 999 / 4096
 */
void TIM3_WaveOut_Init(void);
void TIM3_WaveOut_Set(uint16_t adc_val);

#ifdef __cplusplus
}
#endif

#endif /* __TIMER_H */
