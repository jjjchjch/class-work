#ifndef __BSP_AD9833_H
#define __BSP_AD9833_H

#include "main.h"
#include <stdio.h>

/*****************************************************************************
 ** 引脚定义 (软件模拟 SPI)
**   AD9833_FSYNC => PB12
**   AD9833_CLK   => PB13
**   AD9833_DATA  => PB15
****************************************************************************/
#define  AD9833_CS_GPIO                GPIOB
#define  AD9833_CS_PIN                 GPIO_PIN_12

#define  AD9833_SCK_GPIO               GPIOB
#define  AD9833_SCK_PIN                GPIO_PIN_13

#define  AD9833_DATA_GPIO              GPIOB
#define  AD9833_DATA_PIN               GPIO_PIN_15


/*****************************************************************************
 ** 声明全局函数
****************************************************************************/
/* WaveMode */
#define AD9833_OUT_SINUS    ((0 << 5) | (0 << 1) | (0 << 3))
#define AD9833_OUT_TRIANGLE ((0 << 5) | (1 << 1) | (0 << 3))
#define AD9833_OUT_MSB      ((1 << 5) | (0 << 1) | (1 << 3))
#define AD9833_OUT_MSB2     ((1 << 5) | (0 << 1) | (0 << 3))  /* 输出频率低一半的方波 */

/* Registers */
#define AD9833_REG_CMD      (0 << 14)
#define AD9833_REG_FREQ0    (1 << 14)
#define AD9833_REG_FREQ1    (2 << 14)
#define AD9833_REG_PHASE0   (6 << 13)
#define AD9833_REG_PHASE1   (7 << 13)

/* Command Control Bits */
#define AD9833_B28          (1 << 13)
#define AD9833_HLB          (1 << 12)
#define AD9833_FSEL0        (0 << 11)
#define AD9833_FSEL1        (1 << 11)
#define AD9833_PSEL0        (0 << 10)
#define AD9833_PSEL1        (1 << 10)
#define AD9833_PIN_SW       (1 << 9)
#define AD9833_RESET        (1 << 8)
#define AD9833_CLEAR_RESET  (0 << 8)
#define AD9833_SLEEP1       (1 << 7)
#define AD9833_SLEEP12      (1 << 6)
#define AD9833_OPBITEN      (1 << 5)
#define AD9833_SIGN_PIB     (1 << 4)
#define AD9833_DIV2         (1 << 3)
#define AD9833_MODE         (1 << 1)

void AD9833_Init(void);
void AD9833_WriteData(unsigned short txdata);
void AD9833_SetFrequency(unsigned short reg, double fout);
void AD9833_SetPhase(unsigned short reg, unsigned short val);
void AD9833_SetWave(unsigned int WaveMode, unsigned int Freq_SFR, unsigned int Phase_SFR);
void AD9833_Setup(unsigned int Freq_SFR, double Freq, unsigned int Phase_SFR, unsigned int Phase, unsigned int WaveMode);

#endif /* __BSP_AD9833_H */
