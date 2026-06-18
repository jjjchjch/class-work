/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       W25Q128 SPI 读写验证实验 (LCD + UART 双输出)
 *
 *              硬件: STM32F407VET6 + ILI9341 2.8寸 LCD (FSMC, 240×320, RGB565)
 *                    W25Q128 Flash: CS=PC13, SCK=PA5, MISO=PA6, MOSI=PA7 (SPI1)
 *                    KEY1=PA0(下拉,按下高), KEY2=PA1(上拉,按下低)
 *                    UART1: PA9(TX), PA10(RX), 115200-8-N-1
 *
 *              功能:
 *              - KEY1: 向 W25Q128 0x1000 写入 "2023014085 金成昊"(GBK)
 *              - KEY2: 从 W25Q128 0x1000 读出, 逐字节比对验证
 *              - LCD 显示实时数据和操作结果 (中文用拼音 JinChengHao)
 *              - 串口同步打印详细 Hex 数据和状态
 *
 *              系统时钟: 16MHz HSI
 ****************************************************************************************************
 */

#include "main.h"
#include "key.h"
#include "delay.h"
#include "uart.h"
#include "bsp_LCD_ILI9341.h"
#include "bsp_W25Q128.h"
#include <string.h>
#include <stdio.h>

/*===========================================================================
 * 常量定义
 *===========================================================================*/
#define TEST_ADDR           0x00001000  /* W25Q128 写入地址 (避开字库区) */

/* 测试数据: "2023014085 金成昊" — GBK 编码 */
/* LCD 上中文无法显示, 用拼音 JinChengHao 代替 */
static const uint8_t g_testData[] = {
    '2', '0', '2', '3', '0', '1', '4', '0', '8', '5', ' ',
    0xBD, 0xF0,     /* 金 (GBK) */
    0xB3, 0xC9,     /* 成 (GBK) */
    0xEA, 0xBB,     /* 昊 (GBK) */
};
#define TEST_DATA_LEN   (sizeof(g_testData))

/* 拼音版字符串, 供 LCD 显示 */
#define TEST_STR_PINYIN "2023014085 JinChengHao"

/* 读写缓冲区 */
static uint8_t g_readBuf[TEST_DATA_LEN];

/*===========================================================================
 * 操作状态
 *===========================================================================*/
typedef enum {
    OP_NONE = 0,
    OP_WRITE_OK,
    OP_WRITE_FAIL,
    OP_READ_OK,
    OP_READ_FAIL,
} OpStatus_t;

static volatile OpStatus_t g_lastOp = OP_NONE;

/*===========================================================================
 * 串口打印辅助 (UART1, 115200bps, PA9/PA10)
 *===========================================================================*/

static void UART_Println(const char *msg)
{
    UART1_SendString(msg);
    UART1_SendString("\r\n");
}

static void UART_PrintHex(const char *label, const uint8_t *data, uint16_t len)
{
    char buf[8];
    UART1_SendString(label);
    for (uint16_t i = 0; i < len; i++)
    {
        sprintf(buf, "%02X ", data[i]);
        UART1_SendString(buf);
    }
    UART1_SendString("\r\n");
}

static void UART_PrintSep(void)
{
    UART1_SendString("----------------------------------------\r\n");
}

/*===========================================================================
 * LCD 辅助函数
 *===========================================================================*/

/**
 * @brief  绘制 LCD 主界面框架 (只画一次)
 */
static void LCD_DrawUI(void)
{
    LCD_Fill(0, 0, LCD_WIDTH - 1, LED_HEIGHT - 1, BLACK);

    /* 标题栏 */
    LCD_Fill(0, 0, LCD_WIDTH - 1, 30, 0x2104);
    LCD_String(30, 4, (char *)"W25Q128 R/W Test", 16, WHITE, 0x2104);
    LCD_Fill(0, 30, LCD_WIDTH - 1, 32, GREEN);

    /* 系统信息 */
    LCD_String(5, 38,  (char *)"MCU:STM32F407  LCD:ILI9341", 12, CYAN, BLACK);
    LCD_String(5, 54,  (char *)"SPI1:PA5/6/7  CS:PC13", 12, CYAN, BLACK);
    LCD_String(5, 70,  (char *)"UART1:PA9/10  115200bps", 12, CYAN, BLACK);
    LCD_Fill(0, 88, LCD_WIDTH - 1, 90, GREEN);

    /* 操作提示 */
    LCD_String(5, 96,  (char *)"[KEY1] Write  [KEY2] Read+Verify", 16, WHITE, BLACK);
    LCD_Fill(0, 120, LCD_WIDTH - 1, 122, GREEN);

    /* 数据预览 */
    LCD_String(5, 128, (char *)"Test Data:", 12, YELLOW, BLACK);
    LCD_String(5, 148, (char *)TEST_STR_PINYIN, 16, 0xFFE0, BLACK);

    /* 结果区标签 */
    LCD_Fill(0, 180, LCD_WIDTH - 1, 182, GREEN);
    LCD_String(5, 188, (char *)"Result:", 12, YELLOW, BLACK);
}

/**
 * @brief  清除 LCD 结果区域
 */
static void LCD_ClearResultArea(void)
{
    LCD_Fill(0, 204, LCD_WIDTH - 1, LED_HEIGHT - 1, BLACK);
}

/**
 * @brief  LCD 显示操作中的数据 (拼音)
 */
static void LCD_ShowOpData(const char *opName, uint16_t color)
{
    LCD_ClearResultArea();

    char buf[48];
    uint16_t y = 210;

    sprintf(buf, "[%s]", opName);
    LCD_String(5, y, buf, 16, color, BLACK);
    y += 24;

    LCD_String(5, y, (char *)TEST_STR_PINYIN, 16, WHITE, BLACK);
    y += 22;

    sprintf(buf, "Addr:0x%08lX", (unsigned long)TEST_ADDR);
    LCD_String(5, y, buf, 12, 0x8410, BLACK);
}

/**
 * @brief  LCD 显示验证结果 (通过/失败)
 */
static void LCD_ShowVerifyResult(uint8_t passed)
{
    LCD_ClearResultArea();
    uint16_t y = 210;

    if (passed)
    {
        LCD_String(5, y, (char *)"[VERIFY] PASSED!", 16, GREEN, BLACK);
        y += 24;
        LCD_String(5, y, (char *)"Read == Write  OK", 12, GREEN, BLACK);
        y += 22;
        LCD_String(5, y, (char *)TEST_STR_PINYIN, 16, WHITE, BLACK);
    }
    else
    {
        LCD_String(5, y, (char *)"[VERIFY] FAILED!", 16, RED, BLACK);
        y += 24;
        LCD_String(5, y, (char *)"Mismatch! See UART", 12, RED, BLACK);
    }
}

/*===========================================================================
 * W25Q128 操作 (含 LCD + UART 双输出)
 *===========================================================================*/

/**
 * @brief  KEY1: 写入数据并回读确认
 * @retval 0=成功, 1=失败
 */
static uint8_t W25Q128_DoWrite(void)
{
    if (xW25Q128.FlagInit == 0)
    {
        UART_Println("[WRITE] ERROR: W25Q128 not init!");
        return 1;
    }

    /* ---- 串口打印写入信息 ---- */
    UART_PrintSep();
    UART_Println(">>> [KEY1] WRITE Operation <<<");
    UART_PrintHex("    Write Data  : ", g_testData, TEST_DATA_LEN);
    {
        char buf[48];
        sprintf(buf, "    Address     : 0x%08lX", (unsigned long)TEST_ADDR);
        UART_Println(buf);
        sprintf(buf, "    Length      : %u bytes", (unsigned int)TEST_DATA_LEN);
        UART_Println(buf);
    }
    UART_Println("    Content(PY) : 2023014085 JinChengHao");

    /* ---- LCD 显示正在写入的数据 ---- */
    LCD_ShowOpData("WRITE", YELLOW);

    /* ---- 执行写入 ---- */
    W25Q128_WriteData(TEST_ADDR, (uint8_t *)g_testData, TEST_DATA_LEN);
    UART_Println("    -> W25Q128_WriteData() done");

    /* ---- 回读验证 ---- */
    uint8_t verifyBuf[TEST_DATA_LEN];
    memset(verifyBuf, 0, TEST_DATA_LEN);
    W25Q128_ReadData(TEST_ADDR, verifyBuf, TEST_DATA_LEN);

    UART_PrintHex("    Read Back   : ", verifyBuf, TEST_DATA_LEN);

    if (memcmp(g_testData, verifyBuf, TEST_DATA_LEN) == 0)
    {
        UART_Println("    [WRITE] SUCCESS! Data verified.");
        LCD_ShowVerifyResult(1);
        UART_PrintSep();
        return 0;
    }
    else
    {
        UART_Println("    [WRITE] FAILED! Data mismatch.");
        for (uint16_t i = 0; i < TEST_DATA_LEN; i++)
        {
            if (g_testData[i] != verifyBuf[i])
            {
                char buf[64];
                sprintf(buf, "    MISMATCH @ byte[%u]: Wrote=0x%02X Read=0x%02X",
                        i, g_testData[i], verifyBuf[i]);
                UART_Println(buf);
            }
        }
        LCD_ShowVerifyResult(0);
        UART_PrintSep();
        return 1;
    }
}

/**
 * @brief  KEY2: 读取数据并与原始数据比对
 * @retval 0=一致, 1=不一致
 */
static uint8_t W25Q128_DoReadVerify(void)
{
    if (xW25Q128.FlagInit == 0)
    {
        UART_Println("[READ] ERROR: W25Q128 not init!");
        return 1;
    }

    /* ---- 串口打印读取信息 ---- */
    UART_PrintSep();
    UART_Println(">>> [KEY2] READ + VERIFY Operation <<<");
    {
        char buf[48];
        sprintf(buf, "    Address     : 0x%08lX", (unsigned long)TEST_ADDR);
        UART_Println(buf);
        sprintf(buf, "    Length      : %u bytes", (unsigned int)TEST_DATA_LEN);
        UART_Println(buf);
    }
    UART_PrintHex("    Expected    : ", g_testData, TEST_DATA_LEN);
    UART_Println("    Expected(PY): 2023014085 JinChengHao");

    /* ---- 执行读取 ---- */
    memset(g_readBuf, 0, TEST_DATA_LEN);
    W25Q128_ReadData(TEST_ADDR, g_readBuf, TEST_DATA_LEN);

    UART_PrintHex("    Read Data   : ", g_readBuf, TEST_DATA_LEN);
    UART_Println("    Read Str(PY): 2023014085 JinChengHao");

    /* ---- LCD 显示读出的数据 ---- */
    LCD_ShowOpData("READ", CYAN);

    /* ---- 比对 ---- */
    if (memcmp(g_testData, g_readBuf, TEST_DATA_LEN) == 0)
    {
        UART_Println("    [VERIFY] PASSED! Data matches.");
        LCD_ShowVerifyResult(1);
        UART_PrintSep();
        return 0;
    }
    else
    {
        UART_Println("    [VERIFY] FAILED! Data mismatch.");
        for (uint16_t i = 0; i < TEST_DATA_LEN; i++)
        {
            if (g_testData[i] != g_readBuf[i])
            {
                char buf[64];
                sprintf(buf, "    MISMATCH @ byte[%u]: Exp=0x%02X Got=0x%02X",
                        i, g_testData[i], g_readBuf[i]);
                UART_Println(buf);
            }
        }
        LCD_ShowVerifyResult(0);
        UART_PrintSep();
        return 1;
    }
}

/*===========================================================================
 * 主函数
 *===========================================================================*/

static void SystemClock_Config(void);

int main(void)
{
    uint8_t keyVal;
    uint8_t result;

    /* ---- 硬件初始化 ---- */
    HAL_Init();
    SystemClock_Config();

    /* ---- UART1 初始化 (115200bps, PA9/PA10) ---- */
    UART1_Init();
    UART_PrintSep();
    UART_Println("=== W25Q128 R/W Test Start ===");
    UART_Println("MCU : STM32F407VET6 @ 16MHz HSI");
    UART_Println("LCD : ILI9341 240x320 (FSMC)");
    UART_Println("SPI : PA5=SCK PA6=MISO PA7=MOSI PC13=CS");
    UART_Println("UART: PA9=TX PA10=RX 115200-8-N-1");
    UART_PrintSep();

    /* ---- LCD 初始化 ---- */
    LCD_Init();
    LCD_DrawUI();
    UART_Println("LCD Init OK");

    /* ---- 按键初始化 ---- */
    key_init();
    UART_Println("KEY Init OK (KEY1=PA0 KEY2=PA1)");

    /* ---- W25Q128 初始化 ---- */
    uint8_t w25q_ok = W25Q128_Init();

    if (w25q_ok)
    {
        char buf[48];
        sprintf(buf, "W25Q128 Init OK [%s]", xW25Q128.type);
        UART_Println(buf);
        if (xW25Q128.FlagGBKStorage)
        {
            UART_Println("GBK Font Storage: Detected");
        }
        LCD_String(5, 222, buf, 12, GREEN, BLACK);
    }
    else
    {
        UART_Println("W25Q128 Init FAILED! Check wiring.");
        LCD_String(5, 222, (char *)"W25Q128 Init FAIL!", 12, RED, BLACK);
    }
    UART_PrintSep();

    /* ---- 主循环 ---- */
    while (1)
    {
        keyVal = key_scan(0);   /* 单次触发, 不支持连按 */

        if (keyVal == KEY1_PRES)
        {
            result = W25Q128_DoWrite();
            g_lastOp = (result == 0) ? OP_WRITE_OK : OP_WRITE_FAIL;
        }
        else if (keyVal == KEY2_PRES)
        {
            result = W25Q128_DoReadVerify();
            g_lastOp = (result == 0) ? OP_READ_OK : OP_READ_FAIL;
        }

        HAL_Delay(20);
    }
}

/*===========================================================================
 * 系统时钟配置: HSI 16MHz, 无 PLL
 *===========================================================================*/
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

/*===========================================================================
 * 错误处理
 *===========================================================================*/
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

