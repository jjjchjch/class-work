/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include "gpio.h"
#include "key.h"
#include "stm32f4xx_it.h"
#include "uart.h"
#include "time.h"
#include "clock.h"
#include <stdio.h>



int main(void)
{
	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();
	key_init();
	UART1_Init();
	Time_Init();

	

	Time_Start();

	while (1)
	{
		if (TIM4_GetElapsed() != 0U)
		{
			HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_GPIO_PIN);
		}

		if (TIM5_GetElapsed() != 0U)
		{
			HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_GPIO_PIN);
			TimeKeeper_Tick();
			Print_CurrentTime();
		}
	}
}

void Error_Handler(void)
{
	__disable_irq();
	while (1)
	{
	}
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
	(void)file;
	(void)line;
}
#endif /* USE_FULL_ASSERT */
