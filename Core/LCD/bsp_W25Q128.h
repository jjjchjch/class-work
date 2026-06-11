#ifndef __BSP__W25Q_H
#define __BSP__W25Q_H
/***********************************************************************************************************************************
 ** 【文件名称】  bsp_W25Q128.h
 ** 【编写人员】  魔女开发板团队
 ** 【淘    宝】  魔女开发板      https://demoboard.taobao.com
 ***********************************************************************************************************************************
 ** 【功能描述】  定义引脚、定义全局结构体、声明全局函数
 **
 ** 【适用平台】  STM32F407 + keil5 + HAL库
 **
 ** 【 CubeMX 】  为了移植方便，本示例另写了 bsp_W25Q128.c 和 bsp_W25Q128.h 两个驱动文件
 **               无需使用CubeMX配置SPI引脚，直接调用 bsp_W25Q128.h 所声明的函数，即可实现：初始化、读取、存储
 **
 ** 【代码使用】  所用GPIO引脚和SPI端口，可以在 "bsp_W25Q128.h"中修改;
 **               整个W25Q128的读写操作，包括中文字库数据，已封装成4个全局函数，只用这4个函数，即可完成对其存取操作
 **               初始化  ：  W25Q128_Init()
 **               读取数据：  W25Q128_ReadData (uint32_t addr, uint8_t *pData, uint16_t num)
 **               写入数据：  W25Q128_WriteData(uint32_t addr, uint8_t *pData, uint16_t num)
 **               字模读取：  W25Q128_ReadFontData(uint8_t *typeface, uint8_t size, uint8_t *dataBuf)
 **
 ** 【划 重 点】 1_ 本代码，适用W25Q16、W25Q32、W25Q65、W25Q128，不适用于W25Q256;
 **              2_ 如果使用的是魔女开发板上的W25Q128，存储数据时，请使用前10M空间地址0X0~0x9FFFFF;
 **                 芯片后6M空间,已存储汉字字模数据，地址为0x00A00000~0x01000000;
 **              3_ 代码已经多次优化，直接使用即可
 **
 ** 【字库使用】  特别地注意，请慎重使用芯片擦除，魔女开发板的w25q128，在存储区尾部已烧录宋体4种字号大小汉字GBK字模数据
 **               字库存放地址：0x00A00000 - 0x01000000   尽量不要写操作此区域地址
 **
***********************************************************************************************************************************/
#include "stm32f4xx_hal.h"
#include <stdio.h>

/*****************************************************************************
 ** 引脚定义
****************************************************************************/
#define  W25Q128_SPI                    SPI1
#define  W25Q128_SPI_AFx                GPIO_AF5_SPI1

#define  W25Q128_SCK_GPIO               GPIOA
#define  W25Q128_SCK_PIN                GPIO_PIN_5

#define  W25Q128_MISO_GPIO              GPIOA
#define  W25Q128_MISO_PIN               GPIO_PIN_6

#define  W25Q128_MOSI_GPIO              GPIOA
#define  W25Q128_MOSI_PIN               GPIO_PIN_7

#define  W25Q128_CS_GPIO                GPIOC
#define  W25Q128_CS_PIN                 GPIO_PIN_13

#define  GBK_STORAGE_ADDR               0x00A00000

/*****************************************************************************
 ** 声明全局变量
****************************************************************************/
typedef struct
{
    uint8_t   FlagInit;
    uint8_t   FlagGBKStorage;
    char      type[20];
    uint16_t  StartupTimes;
} xW25Q_TypeDef;

extern xW25Q_TypeDef  xW25Q128;

/*****************************************************************************
 ** 声明全局函数
****************************************************************************/
uint8_t W25Q128_Init(void);
void    W25Q128_ReadData(uint32_t addr, uint8_t *pData, uint16_t num);
void    W25Q128_WriteData(uint32_t addr, uint8_t *pData, uint16_t num);
void    W25Q128_ReadFontData(uint8_t *pFont, uint8_t size, uint8_t *fontData);

#endif
