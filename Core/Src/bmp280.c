/**
 ****************************************************************************************************
 * @file        bmp280.c
 * @brief       BMP280 气压传感器驱动
 *              移植自 "进阶与提高2 BMP280实验" 的 BSP/BMP280/bmp280.c
 *
 *              硬件: SCL=PE4, SDA=PE6, SDO 默认接地
 ****************************************************************************************************
 */

#include "bmp280.h"
#include "myiic.h"
#include "delay.h"
#include <math.h>
#include <stdio.h>

extern char g_uartBuf[128];
extern void UART1_SendString(const char *text);

unsigned short dig_T1;
short dig_T2;
short dig_T3;
unsigned short dig_P1;
short dig_P2;
short dig_P3;
short dig_P4;
short dig_P5;
short dig_P6;
short dig_P7;
short dig_P8;
short dig_P9;

#define MSLP                    101325      /* Mean Sea Level Pressure = 1013.25 hPA */
#define ALTITUDE_OFFSET         10000

int32_t gs32Pressure0 = MSLP;

/* ---- 写 BMP280 寄存器 ---- */
void Bmp280WriteByte(uint8_t addr, uint8_t dat)
{
    IIC_Start();
    IIC_Send_Byte(BMP280_ADDRESS << 1);     /* 从机地址 + 写信号 */
    IIC_Wait_Ack();
    IIC_Send_Byte(addr);
    IIC_Wait_Ack();
    IIC_Send_Byte(dat);
    IIC_Wait_Ack();
    IIC_Stop();
}

/* ---- 读 BMP280 寄存器 ---- */
uint8_t Bmp280ReadByte(uint8_t addr)
{
    uint8_t dat;

    IIC_Start();
    IIC_Send_Byte(BMP280_ADDRESS << 1 | 0); /* 从机地址 + 写信号 */
    IIC_Wait_Ack();
    IIC_Send_Byte(addr);
    IIC_Wait_Ack();

    IIC_Start();
    IIC_Send_Byte(BMP280_ADDRESS << 1 | 1); /* 从机地址 + 读信号 */
    IIC_Wait_Ack();
    dat = IIC_Read_Byte(0);                 /* 无需应答 */
    IIC_Stop();

    return dat;
}

/* ---- 连续读 3 字节 (温度/气压数据) ---- */
long bmp280_MultipleReadThree(unsigned char addr)
{
    unsigned char msb, lsb, xlsb;
    long temp = 0;

    msb = Bmp280ReadByte(addr);
    lsb = Bmp280ReadByte(addr + 1);
    xlsb = Bmp280ReadByte(addr + 2);

    temp = (long)(((unsigned long)msb << 12) | ((unsigned long)lsb << 4) | ((unsigned long)xlsb >> 4));

    return temp;
}

/* ---- 连续读 2 字节 (校准参数, 小端序: addr=LSB, addr+1=MSB) ---- */
short bmp280_MultipleReadTwo(unsigned char addr)
{
    unsigned char byteL, byteH;
    short temp = 0;

    byteL = Bmp280ReadByte(addr);       /* LSB */
    byteH = Bmp280ReadByte(addr + 1);   /* MSB */

    temp = (short)(((unsigned short)byteH << 8) | (unsigned short)byteL);

    return temp;
}

/* ---- BMP280 初始化 ---- */
void Bmp280Init(void)
{
    uint8_t id;

    IIC_Init();

    Bmp280WriteByte(BMP280_RESET_REG, 0xB6);    /* 软复位 */
    id = Bmp280ReadByte(BMP280_CHIPID_REG);      /* 读取 ID */

    if (id == BMP280_CHIPID)
    {
        sprintf(g_uartBuf, "# bmp280 id OK (0x%02X)\r\n", id);
        UART1_SendString(g_uartBuf);
    }
    else
    {
        sprintf(g_uartBuf, "# bmp280 id ERR: 0x%02X\r\n", id);
        UART1_SendString(g_uartBuf);
    }

    Bmp280WriteByte(0xF4, 0xFF);
    Bmp280WriteByte(0xF5, 0x00);

    dig_T1 = bmp280_MultipleReadTwo(0x88);
    dig_T2 = bmp280_MultipleReadTwo(0x8A);
    dig_T3 = bmp280_MultipleReadTwo(0x8C);
    dig_P1 = bmp280_MultipleReadTwo(0x8E);
    dig_P2 = bmp280_MultipleReadTwo(0x90);
    dig_P3 = bmp280_MultipleReadTwo(0x92);
    dig_P4 = bmp280_MultipleReadTwo(0x94);
    dig_P5 = bmp280_MultipleReadTwo(0x96);
    dig_P6 = bmp280_MultipleReadTwo(0x98);
    dig_P7 = bmp280_MultipleReadTwo(0x9A);
    dig_P8 = bmp280_MultipleReadTwo(0x9C);
    dig_P9 = bmp280_MultipleReadTwo(0x9E);

    sprintf(g_uartBuf, "# Calib T: %u %d %d\r\n", dig_T1, dig_T2, dig_T3);
    UART1_SendString(g_uartBuf);
    sprintf(g_uartBuf, "# Calib P: %u %d %d %d %d %d %d %d %d\r\n",
           dig_P1, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9);
    UART1_SendString(g_uartBuf);

    delay_ms(200);
}

/* ---- 全局数据 ---- */
Bmp280DataTypeDef Bmp280Data;

/* ---- 获取 BMP280 温度与气压值 ---- */
uint8_t bmp280_GetValue(void)
{
    long adc_T;
    long adc_P;
    long var1, var2, t_fine, P;

    adc_T = bmp280_MultipleReadThree(0xFA); /* 0xFA 0xFB 0xFC */
    adc_P = bmp280_MultipleReadThree(0xF7); /* 0xF7 0xF8 0xF9 */

    if ((adc_P == 0) | (adc_T == 0))
    {
        sprintf(g_uartBuf, "[BMP] read err: adc_T=%ld adc_P=%ld\r\n", adc_T, adc_P);
        UART1_SendString(g_uartBuf);
        return 0;
    }

    /* Temperature */
    var1 = (((double)adc_T) / 16384.0 - ((double)dig_T1) / 1024.0) * ((double)dig_T2);
    var2 = ((((double)adc_T) / 131072.0 - ((double)dig_T1) / 8192.0) *
            (((double)adc_T) / 131072.0 - ((double)dig_T1) / 8192.0)) * ((double)dig_T3);

    t_fine = (unsigned long)(var1 + var2);
    Bmp280Data.T = (var1 + var2) / 5120.0;

    /* Pressure */
    var1 = ((double)t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)dig_P4) * 65536.0);
    var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)dig_P1);
    P = 1048576.0 - (double)adc_P;
    P = (P - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)dig_P9) * P * P / 2147483648.0;
    var2 = P * ((double)dig_P8) / 32768.0;
    Bmp280Data.P = (P + (var1 + var2 + ((double)dig_P7)) / 16.0);

    return 1;
}

/* ---- 计算海拔高度 ---- */
void BMP280_CalculateAbsoluteAltitude(int32_t *pAltitude, int32_t PressureVal)
{
    *pAltitude = 4433000 * (1 - pow((PressureVal / (float)gs32Pressure0), 0.1903));
}
