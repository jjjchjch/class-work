/**
 ****************************************************************************************************
 * @file        myiic.c
 * @brief       软件 IIC 驱动 (PE4=SCL, PE6=SDA)
 *              移植自 "进阶与提高2 BMP280实验" 的 BSP/IIC/myiic.c
 ****************************************************************************************************
 */

#include "myiic.h"

/* ---- 微秒延时 (SysTick 轮询, 兼容 HAL_Delay) ---- */
static void delay_us(uint32_t nus)
{
    uint32_t ticks = nus * (SystemCoreClock / 1000000);
    uint32_t told = SysTick->VAL;
    uint32_t tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;

    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if (tcnt >= ticks)
                break;
        }
    }
}

/* ---- IIC 初始化 ---- */
void IIC_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    IIC_SCL_GPIO_CLK_ENABLE();
    IIC_SDA_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = IIC_SCL_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IIC_SCL_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = IIC_SDA_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(IIC_SDA_GPIO_PORT, &gpio_init_struct);

    IIC_Stop();
}

/* ---- 产生 IIC 起始信号 ---- */
void IIC_Start(void)
{
    IIC_SDA(1);
    IIC_SCL(1);
    delay_us(4);
    IIC_SDA(0);
    delay_us(4);
    IIC_SCL(0);
}

/* ---- 产生 IIC 停止信号 ---- */
void IIC_Stop(void)
{
    IIC_SCL(0);
    IIC_SDA(0);
    delay_us(4);
    IIC_SCL(1);
    IIC_SDA(1);
    delay_us(4);
}

/* ---- 等待应答信号 ---- */
uint8_t IIC_Wait_Ack(void)
{
    uint8_t ucErrTime = 0;

    IIC_SDA(1);
    delay_us(1);
    IIC_SCL(1);
    delay_us(1);

    while (IIC_READ_SDA)
    {
        ucErrTime++;
        if (ucErrTime > 250)
        {
            IIC_Stop();
            return 1;
        }
    }
    IIC_SCL(0);
    return 0;
}

/* ---- 产生 ACK 应答 ---- */
void IIC_Ack(void)
{
    IIC_SCL(0);
    IIC_SDA(0);
    delay_us(2);
    IIC_SCL(1);
    delay_us(2);
    IIC_SCL(0);
}

/* ---- 不产生 ACK 应答 ---- */
void IIC_NAck(void)
{
    IIC_SCL(0);
    IIC_SDA(1);
    delay_us(2);
    IIC_SCL(1);
    delay_us(2);
    IIC_SCL(0);
}

/* ---- IIC 发送一个字节 ---- */
void IIC_Send_Byte(uint8_t txd)
{
    uint8_t t;

    IIC_SCL(0);
    for (t = 0; t < 8; t++)
    {
        IIC_SDA((txd & 0x80) >> 7);
        txd <<= 1;
        delay_us(2);
        IIC_SCL(1);
        delay_us(2);
        IIC_SCL(0);
        delay_us(2);
    }
}

/* ---- IIC 读取一个字节 ---- */
uint8_t IIC_Read_Byte(unsigned char ack)
{
    unsigned char i, receive = 0;

    for (i = 0; i < 8; i++)
    {
        IIC_SCL(0);
        delay_us(2);
        IIC_SCL(1);
        receive <<= 1;
        if (IIC_READ_SDA)
            receive++;
        delay_us(1);
    }

    if (!ack)
        IIC_NAck();
    else
        IIC_Ack();

    return receive;
}

/* ---- 向从设备写入一个字节 ---- */
void IIC_Write_One_Byte(uint8_t daddr, uint8_t addr, uint8_t data)
{
    IIC_Start();
    IIC_Send_Byte(daddr);
    IIC_Wait_Ack();
    IIC_Send_Byte(addr);
    IIC_Wait_Ack();
    IIC_Send_Byte(data);
    IIC_Wait_Ack();
    IIC_Stop();
}

/* ---- 从从设备读取一个字节 ---- */
uint8_t IIC_Read_One_Byte(uint8_t daddr, uint8_t addr)
{
    uint8_t temp;

    IIC_Start();
    IIC_Send_Byte(daddr);
    IIC_Wait_Ack();
    IIC_Send_Byte(addr);
    IIC_Wait_Ack();

    IIC_Start();
    IIC_Send_Byte(daddr | 1);
    IIC_Wait_Ack();
    temp = IIC_Read_Byte(0);
    IIC_Stop();

    return temp;
}
