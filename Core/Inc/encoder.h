#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "main.h"

/* ---------------------------------------------------------------
 * EC11 旋转编码器引脚定义
 *
 *  CLK (B相) : PA7 — EXTI7 → EXTI9_5_IRQn  抢占优先级 1 (最高)
 *               CLK 下降沿触发中断，读 DT 判断方向
 *  DT  (A相) : PA6 — 普通输入，仅在 CLK ISR 内读取，不触发中断
 *  SW  (按键) : PC4 — EXTI4 → EXTI4_IRQn    抢占优先级 2 (次级)
 *
 * 方向判断 (CLK 下降沿时):
 *   DT = 1 (HIGH) → 正转 → 切换 LED1 (PC5)
 *   DT = 0 (LOW)  → 逆转 → 切换 LED2 (PB1)
 * 按键:
 *   SW 低电平 → 切换 LED3 (PB2)
 * --------------------------------------------------------------- */

#define ENC_CLK_GPIO_PORT       GPIOA
#define ENC_CLK_GPIO_PIN        GPIO_PIN_7
#define ENC_CLK_GPIO_CLK_EN()   __HAL_RCC_GPIOA_CLK_ENABLE()

#define ENC_DT_GPIO_PORT        GPIOA
#define ENC_DT_GPIO_PIN         GPIO_PIN_6
#define ENC_DT_GPIO_CLK_EN()    __HAL_RCC_GPIOA_CLK_ENABLE()

#define ENC_SW_GPIO_PORT        GPIOC
#define ENC_SW_GPIO_PIN         GPIO_PIN_4
#define ENC_SW_GPIO_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()

void encoder_init(void);

#endif /* __ENCODER_H__ */
