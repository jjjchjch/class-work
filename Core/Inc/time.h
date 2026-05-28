#ifndef __TIME_H__
#define __TIME_H__

#include "main.h"

/* Exported functions prototypes */
void Time_Init(void);
void TimeKeeper_Init(void);
void TimeKeeper_Tick(void);
void Print_CurrentTime(void);
void Time_Start(void);
uint8_t TIM4_GetElapsed(void);
uint8_t TIM5_GetElapsed(void);

#endif /* __TIME_H__ */
