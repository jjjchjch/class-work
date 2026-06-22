/**
 ****************************************************************************************************
 * @file        dsp_test.c
 * @brief       CMSIS-DSP 正余弦序列生成与统计分析实现
 ****************************************************************************************************
 */

#include "dsp_test.h"
#include "arm_math.h"
#include "uart.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* ---- 外部 LCD API ---- */
extern void LCD_Init(void);
extern void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
extern void LCD_String(uint16_t x, uint16_t y, char *pFont, uint8_t size, uint32_t fColor, uint32_t bColor);

/* ---- 全局变量定义 ---- */
float g_sinSeq[DSP_SEQ_LEN];
float g_cosSeq[DSP_SEQ_LEN];

float g_sinMax, g_sinMin, g_sinMean, g_sinRMS;
float g_cosMax, g_cosMin, g_cosMean, g_cosRMS;

/* ---- 颜色定义 ---- */
#define BLACK   0x0000
#define WHITE   0xFFFF
#define YELLOW  0xFFE0
#define CYAN    0x7FFF
#define GREEN   0x07E0
#define RED     0xF800

/*===========================================================================
 * 生成 256 点正/余弦序列
 * 使用 CMSIS-DSP FastMath: arm_sin_f32() / arm_cos_f32()
 *===========================================================================*/
void DSP_GenerateSequences(void)
{
    uint16_t i;
    float step = 2.0f * PI / (float)DSP_SEQ_LEN;  /* 2*PI / 256 */

    for (i = 0; i < DSP_SEQ_LEN; i++)
    {
        float angle = step * (float)i;             /* 0 ~ 2*PI */
        g_sinSeq[i] = arm_sin_f32(angle);
        g_cosSeq[i] = arm_cos_f32(angle);
    }
}

/*===========================================================================
 * 计算统计量: 最大值、最小值、平均值、有效值(RMS)
 * 使用 CMSIS-DSP Statistics 函数
 *===========================================================================*/
void DSP_ComputeStatistics(void)
{
    uint32_t index;  /* 索引 (未使用但 API 需要) */

    /* ---- 正弦序列统计 ---- */
    arm_max_f32(g_sinSeq, DSP_SEQ_LEN, &g_sinMax, &index);
    arm_min_f32(g_sinSeq, DSP_SEQ_LEN, &g_sinMin, &index);
    arm_mean_f32(g_sinSeq, DSP_SEQ_LEN, &g_sinMean);
    arm_rms_f32(g_sinSeq, DSP_SEQ_LEN, &g_sinRMS);

    /* ---- 余弦序列统计 ---- */
    arm_max_f32(g_cosSeq, DSP_SEQ_LEN, &g_cosMax, &index);
    arm_min_f32(g_cosSeq, DSP_SEQ_LEN, &g_cosMin, &index);
    arm_mean_f32(g_cosSeq, DSP_SEQ_LEN, &g_cosMean);
    arm_rms_f32(g_cosSeq, DSP_SEQ_LEN, &g_cosRMS);
}

/*===========================================================================
 * 串口发送波形数据 (兼容 VOFA+ / SerialPlot)
 * 格式: 每个采样点一行, 数值后跟 \r\n
 *
 * ch = 'S': 发送正弦序列
 * ch = 'C': 发送余弦序列
 *===========================================================================*/
void DSP_SendWaveform(char ch)
{
    uint16_t i;
    const float *pData;
    char label[8];

    if (ch == 'S' || ch == 's')
    {
        pData = g_sinSeq;
        strcpy(label, "SIN");
    }
    else
    {
        pData = g_cosSeq;
        strcpy(label, "COS");
    }

    /* 发送数据头 */
    {
        char buf[64];
        sprintf(buf, "=== %s Waveform (%u pts) ===\r\n", label, DSP_SEQ_LEN);
        UART1_SendString(buf);
    }

    /* 逐点发送 (按 VOFA+ Justfire 协议: 数值\r\n) */
    for (i = 0; i < DSP_SEQ_LEN; i++)
    {
        char buf[32];
        /* 将 float (-1~+1) 映射到 0~3300 模拟电压范围 (mV), 便于串口示波器显示 */
        int16_t val = (int16_t)((pData[i] + 1.0f) * 1650.0f);
        if (val < 0)   val = 0;
        if (val > 3300) val = 3300;
        sprintf(buf, "%d\r\n", val);
        UART1_SendString(buf);
    }

    UART1_SendString("=== END ===\r\n");
}

/*===========================================================================
 * LCD 显示计算结果
 * 第一屏: 正弦统计信息
 * 第二屏: 余弦统计信息
 *===========================================================================*/
void DSP_DisplayResults(void)
{
    char buf[64];

    /* 清屏 + 标题栏 */
    LCD_Fill(0, 0, 239, 319, BLACK);
    LCD_Fill(0, 0, 239, 28, 0x2104);  /* 深蓝色标题栏 */
    LCD_String(10, 4, (char *)"DSP Sine/Cosine Statistics", 16, WHITE, 0x2104);
    LCD_Fill(0, 28, 239, 30, GREEN);  /* 分割线 */

    /* ===== 正弦统计 ===== */
    LCD_String(5, 36, (char *)"-- SINE Wave (256 pts) --", 12, YELLOW, BLACK);

    sprintf(buf, "Max:    %+.4f", (double)g_sinMax);
    LCD_String(10, 54, buf, 12, WHITE, BLACK);

    sprintf(buf, "Min:    %+.4f", (double)g_sinMin);
    LCD_String(10, 72, buf, 12, WHITE, BLACK);

    sprintf(buf, "Mean:   %+.4f", (double)g_sinMean);
    LCD_String(10, 90, buf, 12, WHITE, BLACK);

    sprintf(buf, "RMS:     %.4f", (double)g_sinRMS);
    LCD_String(10, 108, buf, 12, CYAN, BLACK);

    /* 理论参考值 */
    LCD_String(10, 126, (char *)"Theory: Max=+1 Min=-1", 12, 0x8410, BLACK);
    LCD_String(10, 140, (char *)"        Mean=0 RMS=0.707", 12, 0x8410, BLACK);

    /* 分割线 */
    LCD_Fill(0, 158, 239, 160, GREEN);

    /* ===== 余弦统计 ===== */
    LCD_String(5, 166, (char *)"-- COSINE Wave (256 pts) --", 12, YELLOW, BLACK);

    sprintf(buf, "Max:    %+.4f", (double)g_cosMax);
    LCD_String(10, 184, buf, 12, WHITE, BLACK);

    sprintf(buf, "Min:    %+.4f", (double)g_cosMin);
    LCD_String(10, 202, buf, 12, WHITE, BLACK);

    sprintf(buf, "Mean:   %+.4f", (double)g_cosMean);
    LCD_String(10, 220, buf, 12, WHITE, BLACK);

    sprintf(buf, "RMS:     %.4f", (double)g_cosRMS);
    LCD_String(10, 238, buf, 12, CYAN, BLACK);

    /* 理论参考值 */
    LCD_String(10, 256, (char *)"Theory: Max=+1 Min=-1", 12, 0x8410, BLACK);
    LCD_String(10, 270, (char *)"        Mean=0 RMS=0.707", 12, 0x8410, BLACK);

    /* 底部操作提示 */
    LCD_Fill(0, 288, 239, 290, GREEN);
    LCD_String(5, 295, (char *)"KEY1:Send SIN  KEY2:Send COS", 12, CYAN, BLACK);
    LCD_String(5, 310, (char *)"KEY3:Exit to Menu", 12, 0x8410, BLACK);
}
