#include "main.h"
#include "gpio.h"
#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "key.h"

static void SystemClock_Config(void);
static uint8_t ProcessCommand(const char *rx_buf);
static void HandleKeyPress(uint8_t key_code);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  UART1_Init();
  key_init();

  char rx_buf[32] = {0};
  uint8_t rx_idx = 0;
  uint8_t ch = 0;
  uint8_t key_val = 0;
  uint32_t last_key_time = 0;

  UART1_SendString("\r\n========== Serial Communication Test ==========\r\n");
  UART1_SendString("Commands: LED1 ON/OFF, LED2 ON/OFF, LED3 ON/OFF\r\n");
  UART1_SendString("Press KEY1/KEY2/KEY3 to send key info via UART\r\n");
  UART1_SendString("================================================\r\n\r\n");

  while (1)
  {
    /* ============ Receive and Echo ============ */
    if (UART1_ReceiveByte(&ch))
    {
      /* Echo the received character */
      char echo_msg[2] = {ch, '\0'};
      UART1_SendString(echo_msg);

      /* Check for command terminator (Enter/CR/LF) */
      if (ch == '\r' || ch == '\n')
      {
        if (rx_idx > 0)
        {
          /* Process the command */
          ProcessCommand(rx_buf);
          /* Clear buffer */
          rx_idx = 0;
          rx_buf[0] = '\0';
        }
        /* Send newline for formatting */
        UART1_SendString("\r\n");
      }
      else if (rx_idx < (sizeof(rx_buf) - 1U))
      {
        /* Add character to buffer */
        rx_buf[rx_idx++] = (char)ch;
        rx_buf[rx_idx] = '\0';
      }
    }

    /* ============ Key Scanning ============ */
    key_val = key_scan(0);
    if (key_val != 0)
    {
      /* Debounce: avoid multiple detections */
      if ((HAL_GetTick() - last_key_time) > 200)
      {
        HandleKeyPress(key_val);
        last_key_time = HAL_GetTick();
      }
    }
  }
}

static uint8_t ProcessCommand(const char *rx_buf)
{
  if (rx_buf == NULL || strlen(rx_buf) == 0)
  {
    return 0;
  }

  /* Check for LED1 commands using strstr() */
  if (strstr(rx_buf, "LED1") != NULL)
  {
    if (strstr(rx_buf, "ON") != NULL)
    {
      HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);
      UART1_SendString("[INFO] LED1 is now ON\r\n");
      return 1;
    }
    else if (strstr(rx_buf, "OFF") != NULL)
    {
      HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_SET);
      UART1_SendString("[INFO] LED1 is now OFF\r\n");
      return 1;
    }
  }

  /* Check for LED2 commands using strstr() */
  if (strstr(rx_buf, "LED2") != NULL)
  {
    if (strstr(rx_buf, "ON") != NULL)
    {
      HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_GPIO_PIN, GPIO_PIN_RESET);
      UART1_SendString("[INFO] LED2 is now ON\r\n");
      return 1;
    }
    else if (strstr(rx_buf, "OFF") != NULL)
    {
      HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_GPIO_PIN, GPIO_PIN_SET);
      UART1_SendString("[INFO] LED2 is now OFF\r\n");
      return 1;
    }
  }

  /* Check for LED3 commands using strstr() */
  if (strstr(rx_buf, "LED3") != NULL)
  {
    if (strstr(rx_buf, "ON") != NULL)
    {
      HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_GPIO_PIN, GPIO_PIN_RESET);
      UART1_SendString("[INFO] LED3 is now ON\r\n");
      return 1;
    }
    else if (strstr(rx_buf, "OFF") != NULL)
    {
      HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_GPIO_PIN, GPIO_PIN_SET);
      UART1_SendString("[INFO] LED3 is now OFF\r\n");
      return 1;
    }
  }

  return 0;
}

static void HandleKeyPress(uint8_t key_code)
{
  switch (key_code)
  {
    case KEY1_PRES:
      UART1_SendString("[KEY] KEY1 Pressed\r\n");
      break;
    case KEY2_PRES:
      UART1_SendString("[KEY] KEY2 Pressed\r\n");
      break;
    case KEY3_PRES:
      UART1_SendString("[KEY] KEY3 Pressed\r\n");
      break;
    default:
      break;
  }
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
