#ifndef __MYIIC_H
#define __MYIIC_H

#include "main.h"
#include <stdint.h>

/* ---- 软件 IIC 引脚定义 (PE4=SCL, PE6=SDA) ---- */

#define IIC_SCL_GPIO_PORT               GPIOE
#define IIC_SCL_GPIO_PIN                GPIO_PIN_4
#define IIC_SCL_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)

#define IIC_SDA_GPIO_PORT               GPIOE
#define IIC_SDA_GPIO_PIN                GPIO_PIN_6
#define IIC_SDA_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)

/* ---- IO 操作宏 ---- */
#define IIC_SCL(x)        do{ x ? \
                              HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)

#define IIC_SDA(x)        do{ x ? \
                              HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)

#define IIC_READ_SDA     HAL_GPIO_ReadPin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN)

/* ---- IIC 操作函数声明 ---- */
void    IIC_Init(void);
void    IIC_Start(void);
void    IIC_Stop(void);
void    IIC_Send_Byte(uint8_t txd);
uint8_t IIC_Read_Byte(unsigned char ack);
uint8_t IIC_Wait_Ack(void);
void    IIC_Ack(void);
void    IIC_NAck(void);
void    IIC_Write_One_Byte(uint8_t daddr, uint8_t addr, uint8_t data);
uint8_t IIC_Read_One_Byte(uint8_t daddr, uint8_t addr);

#endif
