/**
 ****************************************************************************************************
 * @file        bmp280.h
 * @brief       BMP280 气压传感器驱动头文件
 *              移植自 "进阶与提高2 BMP280实验" 的 BSP/BMP280/bmp280.h
 ****************************************************************************************************
 */

#ifndef __BMP280_H
#define __BMP280_H

#include "main.h"

typedef struct
{
    float P;    /* 气压 */
    float T;    /* 温度 */
} Bmp280DataTypeDef;

extern Bmp280DataTypeDef Bmp280Data;

#define BMP280_ADDRESS                      0x76        /* 从设备地址, 编程时需要左移1bit */
#define BMP280_RESET_VALUE                  0xB6        /* 复位寄存器写入值 */
#define BMP280_CHIPID                       0x60        /* Chip ID (育松模块是0x60) */

#define BMP280_CHIPID_REG                   0xD0        /* Chip ID Register */
#define BMP280_RESET_REG                    0xE0        /* Softreset Register */
#define BMP280_STATUS_REG                   0xF3        /* Status Register */
#define BMP280_CTRLMEAS_REG                 0xF4        /* Ctrl Measure Register */
#define BMP280_CONFIG_REG                   0xF5        /* Configuration Register */
#define BMP280_PRESSURE_MSB_REG             0xF7        /* Pressure MSB Register */
#define BMP280_PRESSURE_LSB_REG             0xF8        /* Pressure LSB Register */
#define BMP280_PRESSURE_XLSB_REG            0xF9        /* Pressure XLSB Register */
#define BMP280_TEMPERATURE_MSB_REG          0xFA        /* Temperature MSB Reg */
#define BMP280_TEMPERATURE_LSB_REG          0xFB        /* Temperature LSB Reg */
#define BMP280_TEMPERATURE_XLSB_REG         0xFC        /* Temperature XLSB Reg */

#define BMP280_MEASURING                    0x01
#define BMP280_IM_UPDATE                    0x08

#define BMP280_REGISTER_DIG_T1              0x88
#define BMP280_REGISTER_DIG_T2              0x8A
#define BMP280_REGISTER_DIG_T3              0x8C

#define BMP280_REGISTER_DIG_P1              0x8E
#define BMP280_REGISTER_DIG_P2              0x90
#define BMP280_REGISTER_DIG_P3              0x92
#define BMP280_REGISTER_DIG_P4              0x94
#define BMP280_REGISTER_DIG_P5              0x96
#define BMP280_REGISTER_DIG_P6              0x98
#define BMP280_REGISTER_DIG_P7              0x9A
#define BMP280_REGISTER_DIG_P8              0x9C
#define BMP280_REGISTER_DIG_P9              0x9E

/* ---- 函数声明 ---- */
void    Bmp280Init(void);
uint8_t bmp280_GetValue(void);
void    BMP280_CalculateAbsoluteAltitude(int32_t *pAltitude, int32_t PressureVal);

#endif
