#ifndef __KEY_H__
#define __KEY_H__

#include "main.h"

/* ---------------------------------------------------------------
 * KEY1: PA0, 空闲下拉, 按下 = 高电平
 * KEY2: PA1, 空闲上拉, 按下 = 低电平
 * KEY3: PA4, 空闲上拉, 按下 = 低电平
 * --------------------------------------------------------------- */

#define KEY1_GPIO_PORT          GPIOA
#define KEY1_GPIO_PIN           GPIO_PIN_0
#define KEY1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define KEY2_GPIO_PORT          GPIOA
#define KEY2_GPIO_PIN           GPIO_PIN_1
#define KEY2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define KEY3_GPIO_PORT          GPIOA
#define KEY3_GPIO_PIN           GPIO_PIN_4
#define KEY3_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

/* 按键返回值 */
#define KEY1_PRES   1
#define KEY2_PRES   2
#define KEY3_PRES   3

void    key_init(void);
uint8_t key_scan(uint8_t mode);
void    key_exti_init(void);

#endif /* __KEY_H__ */
