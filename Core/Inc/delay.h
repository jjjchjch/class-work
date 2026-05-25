#ifndef __DELAY_H__
#define __DELAY_H__

#include "main.h"

/* 用 HAL_Delay 实现毫秒延时 */
#define delay_ms(x)   HAL_Delay(x)

#endif /* __DELAY_H__ */
