/**
 ****************************************************************************************************
 * @file        dsp_test.h
 * @brief       CMSIS-DSP 正余弦序列生成与统计分析
 *
 *              功能:
 *              - 使用 arm_sin_f32 / arm_cos_f32 生成 256 点正、余弦序列
 *              - 使用 arm_max_f32 / arm_min_f32 / arm_mean_f32 / arm_rms_f32 统计分析
 *              - 串口示波器输出波形数据
 *              - LCD 显示计算结果
 *
 *              硬件: STM32F407VET6 + ILI9341 LCD + UART1
 ****************************************************************************************************
 */

#ifndef DSP_TEST_H
#define DSP_TEST_H

#include <stdint.h>

#define DSP_SEQ_LEN  1024

/* 正弦序列 (float) */
extern float g_sinSeq[DSP_SEQ_LEN];
/* 余弦序列 (float) */
extern float g_cosSeq[DSP_SEQ_LEN];

/* 统计结果 */
extern float g_sinMax, g_sinMin, g_sinMean, g_sinRMS;
extern float g_cosMax, g_cosMin, g_cosMean, g_cosRMS;

/**
 * @brief  生成 256 点正弦序列和余弦序列 (使用 CMSIS-DSP FastMath)
 *         正弦: sin(2*PI*n/256), 余弦: cos(2*PI*n/256)
 */
void DSP_GenerateSequences(void);

/**
 * @brief  使用 CMSIS-DSP Statistics 函数计算序列的
 *         最大值、最小值、平均值、有效值(RMS)
 */
void DSP_ComputeStatistics(void);

/**
 * @brief  通过串口发送波形数据供上位机 (VOFA+/SerialPlot) 显示
 * @param  ch  'S'=正弦, 'C'=余弦
 */
void DSP_SendWaveform(char ch);

/**
 * @brief  在 LCD 上显示计算结果
 */
void DSP_DisplayResults(void);

#endif /* DSP_TEST_H */
