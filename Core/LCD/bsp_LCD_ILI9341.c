#include "bsp_LCD_ILI9341.h"
#include "stdlib.h"
#include "FONT.H"

/*****************************************************************************
 ** 全局有效    声明、定义
****************************************************************************/
xLCD_TypeDef xLCD = {0};                                    // 管理LCD重要参数

/*****************************************************************************
 ** 本地有效    声明、定义
****************************************************************************/
#define LCD_BL_ON    LCD_BL_GPIO-> BSRR = LCD_BL_PIN;       // 背光引脚，置高电平
#define LCD_BL_OFF   LCD_BL_GPIO-> BSRR = LCD_BL_PIN << 16; // 背光引脚，置低电平

static void setCursor(uint16_t Xpos, uint16_t Ypos);        // 设置光标

volatile typedef struct                                     // LCD地址结构体
{
    uint16_t LCD_REG;
    uint16_t LCD_RAM;
} LCD_TypeDef;
// 使用NOR/SRAM的 Bank1.sector1,地址位HADDR[27,26]=11 A6作为数据命令区分线
#define LCD       ((LCD_TypeDef *) 0x6001FFFE)              // (0x60000000 | 0x0001FFFE)

/* ---- 前置声明 drawAscii，供 LCD_String 使用 ---- */
static void drawAscii(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint32_t bColor, uint32_t fColor);

// 读寄存器
uint16_t readReg(uint16_t LCD_Reg)
{
    LCD->LCD_REG = LCD_Reg;
    HAL_Delay(1);
    return LCD->LCD_RAM;
}

// BGR转换RGB值
uint16_t LCD_BGR2RGB(uint16_t c)
{
    uint16_t   r, g, b, rgb;
    b = (c >> 0) & 0x1f;
    g = (c >> 5) & 0x3f;
    r = (c >> 11) & 0x1f;
    rgb = (b << 11) + (g << 5) + (r << 0);
    return (rgb);
}

// 读取个某点的颜色值
uint16_t LCD_ReadPoint(uint16_t x, uint16_t y)
{
    uint16_t r = 0, g = 0, b = 0;
    if (x >= xLCD.width || y >= xLCD.height) return 0;
    setCursor(x, y);
    LCD->LCD_REG = 0X2E;

    r = LCD->LCD_RAM;                                 // dummy Read
    HAL_Delay(1);
    r = LCD->LCD_RAM;                                 // 实际坐标颜色
    HAL_Delay(1);
    b = LCD->LCD_RAM;
    g = r & 0XFF;
    g <<= 8;
    return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));
}

//LCD开启显示
void LCD_DisplayOn(void)
{
    LCD->LCD_REG = 0X29;
}

//LCD关闭显示
void LCD_DisplayOff(void)
{
    LCD->LCD_REG = 0X28;
}

//设置光标位置
static void setCursor(uint16_t Xpos, uint16_t Ypos)
{
    LCD->LCD_REG = 0X2A;
    LCD->LCD_RAM = Xpos >> 8;
    LCD->LCD_RAM = Xpos & 0XFF;
    LCD->LCD_REG = 0X2B;
    LCD->LCD_RAM = Ypos >> 8;
    LCD->LCD_RAM = Ypos & 0XFF;
}

/******************************************************************
 * 函数名： LCD_DrawPoint
 * 功  能： 画点函数
 *****************************************************************/
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t _color)
{
    LCD->LCD_REG = 0X2A;        // 设置x坐标
    LCD->LCD_RAM = x >> 8;
    LCD->LCD_RAM = x & 0XFF;
    LCD->LCD_REG = 0X2B;        // 设置y坐标
    LCD->LCD_RAM = y >> 8;
    LCD->LCD_RAM = y & 0XFF;
    LCD->LCD_REG = 0X2C;        // 开始写GRAM
    LCD->LCD_RAM = _color;
}

/******************************************************************
 * 函数名： LCD_Init
 * 功  能： 初始化LCD，适用驱动芯片ILI9341
 * 说  明： FSMC时序已适配 16MHz HCLK (HSI)
 *****************************************************************/
void LCD_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct = {0};

    xLCD.FlagInit = 0;

    // 使能GPIO端口
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN
                  | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN;

    // 初始化引脚-背光
    GPIO_InitStruct.Pin   = LCD_BL_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(LCD_BL_GPIO, &GPIO_InitStruct);

    // 通信引脚 GPIOD部分 (FSMC_D0-D3, NOE, NWE, NE1, D13-D15, A6)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5
                        | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10
                        | GPIO_PIN_11 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // 通信引脚 GPIOE部分 (FSMC_D4-D12)
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10
                        | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14
                        | GPIO_PIN_15;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // 使能FSMC时钟
    RCC->AHB3ENR |= RCC_AHB3ENR_FSMCEN;

    // 每个BANK的区域1~4, 要配置3个寄存器
    FSMC_Bank1->BTCR[0]   = 0X00000000;
    FSMC_Bank1->BTCR[0+1] = 0X00000000;
    FSMC_Bank1E->BWTR[0]  = 0X00000000;

    // 操作BCR寄存器，使用异步模式
    FSMC_Bank1->BTCR[0] |= 0x01 << 12;        // 存储器写使能
    FSMC_Bank1->BTCR[0] |= 0x01 << 14;        // 读写使用不同的时序
    FSMC_Bank1->BTCR[0] |= 0x01 << 4;         // 存储器数据宽度为16bit

    /* ---- FSMC 时序（适配 16MHz HCLK, 1 HCLK = 62.5ns）---- */
    // 读时序控制寄存器
    FSMC_Bank1->BTCR[0+1] |= 0x00 << 28;      // 模式A
    FSMC_Bank1->BTCR[0+1] |= 0x02 << 0;       // ADDSET = 2 HCLK = 125ns (>90ns)
    FSMC_Bank1->BTCR[0+1] |= 0x06 << 8;       // DATAST = 6 HCLK = 375ns (>360ns)
    // 写时序控制寄存器
    FSMC_Bank1E->BWTR[0] |= 0x00 << 28;       // 模式A
    FSMC_Bank1E->BWTR[0] |= 0x01 << 0;        // ADDSET = 1 HCLK = 62.5ns (>54ns)
    FSMC_Bank1E->BWTR[0] |= 0x02 << 8;        // DATAST = 2 HCLK = 125ns (>54ns)

    // 使能BANK1，区域1
    FSMC_Bank1->BTCR[0] |= 0x01;

    HAL_Delay(50);
    LCD->LCD_REG = 0x0000;
    LCD->LCD_RAM = 0x0000;
    HAL_Delay(50);
    xLCD.id = readReg(0x0000);

    LCD->LCD_REG = 0XD3;                      // 尝试9341 ID的读取
    xLCD.id = LCD->LCD_RAM;                   // dummy read
    xLCD.id = LCD->LCD_RAM;                   // 读到0X00
    xLCD.id = LCD->LCD_RAM;                   // 读取93
    xLCD.id <<= 8;
    xLCD.id |= LCD->LCD_RAM;                  // 读取41

    // 重新配置写时序控制寄存器的时序（读ID后加速写时序）
    FSMC_Bank1E->BWTR[0] &= ~(0XF << 0);      // 地址建立时间(ADDSET)清零
    FSMC_Bank1E->BWTR[0] &= ~(0XF << 8);      // 数据保存时间清零
    FSMC_Bank1E->BWTR[0] |= 1 << 0;           // ADDSET = 1 HCLK = 62.5ns
    FSMC_Bank1E->BWTR[0] |= 1 << 8;           // DATAST = 1 HCLK = 62.5ns

    // 屏的参数配置（ILI9341 初始化序列）
    LCD->LCD_REG = 0xCF;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0xC1;
    LCD->LCD_RAM = 0X30;
    LCD->LCD_REG = 0xED;
    LCD->LCD_RAM = 0x64;
    LCD->LCD_RAM = 0x03;
    LCD->LCD_RAM = 0X12;
    LCD->LCD_RAM = 0X81;
    LCD->LCD_REG = 0xE8;
    LCD->LCD_RAM = 0x85;
    LCD->LCD_RAM = 0x10;
    LCD->LCD_RAM = 0x7A;
    LCD->LCD_REG = 0xCB;
    LCD->LCD_RAM = 0x39;
    LCD->LCD_RAM = 0x2C;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x34;
    LCD->LCD_RAM = 0x02;
    LCD->LCD_REG = 0xF7;
    LCD->LCD_RAM = 0x20;
    LCD->LCD_REG = 0xEA;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_REG = 0xC0;  // Power control
    LCD->LCD_RAM = 0x1B;  // VRH[5:0]
    LCD->LCD_REG = 0xC1;  // Power control
    LCD->LCD_RAM = 0x01;  // SAP[2:0];BT[3:0]
    LCD->LCD_REG = 0xC5;  // VCM control
    LCD->LCD_RAM = 0x30;
    LCD->LCD_RAM = 0x30;
    LCD->LCD_REG = 0xC7;  // VCM control2
    LCD->LCD_RAM = 0XB7;
    LCD->LCD_REG = 0x36;  // Memory Access Control
    LCD->LCD_RAM = 0x48;
    LCD->LCD_REG = 0x3A;
    LCD->LCD_RAM = 0x55;
    LCD->LCD_REG = 0xB1;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x1A;
    LCD->LCD_REG = 0xB6;  // Display Function Control
    LCD->LCD_RAM = 0x0A;
    LCD->LCD_RAM = 0xA2;
    LCD->LCD_REG = 0xF2;  // 3Gamma Function Disable
    LCD->LCD_RAM = 0x00;
    LCD->LCD_REG = 0x26;  // Gamma curve selected
    LCD->LCD_RAM = 0x01;
    LCD->LCD_REG = 0xE0;  // Set Gamma
    LCD->LCD_RAM = 0x0F;
    LCD->LCD_RAM = 0x2A;
    LCD->LCD_RAM = 0x28;
    LCD->LCD_RAM = 0x08;
    LCD->LCD_RAM = 0x0E;
    LCD->LCD_RAM = 0x08;
    LCD->LCD_RAM = 0x54;
    LCD->LCD_RAM = 0XA9;
    LCD->LCD_RAM = 0x43;
    LCD->LCD_RAM = 0x0A;
    LCD->LCD_RAM = 0x0F;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_REG = 0XE1;   // Set Gamma
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x15;
    LCD->LCD_RAM = 0x17;
    LCD->LCD_RAM = 0x07;
    LCD->LCD_RAM = 0x11;
    LCD->LCD_RAM = 0x06;
    LCD->LCD_RAM = 0x2B;
    LCD->LCD_RAM = 0x56;
    LCD->LCD_RAM = 0x3C;
    LCD->LCD_RAM = 0x05;
    LCD->LCD_RAM = 0x10;
    LCD->LCD_RAM = 0x0F;
    LCD->LCD_RAM = 0x3F;
    LCD->LCD_RAM = 0x3F;
    LCD->LCD_RAM = 0x0F;
    LCD->LCD_REG = 0x2B;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x01;
    LCD->LCD_RAM = 0x3f;
    LCD->LCD_REG = 0x2A;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0x00;
    LCD->LCD_RAM = 0xef;
    LCD->LCD_REG = 0x11;  // 退出睡眠模式
    HAL_Delay(120);
    LCD->LCD_REG = 0x29;  // 打开显示

    LCD_SetDir(0);        // 设置显示的方向（竖屏）
    LCD_Fill(0, 0, xLCD.width, xLCD.height, BLACK);
    LCD_BL_ON;            // 打开LCD背光
    xLCD.FlagInit = 1;
}

/******************************************************************
 * 函数名： LCD_SetDir
 * 功  能： 设置显示方向
 *****************************************************************/
void LCD_SetDir(uint8_t dir)
{
    uint16_t regval = 0;
    uint16_t temp;

    if (dir == 1)
        dir = 6;

    if (dir == 0 || dir == 3)         // 竖屏
    {
        xLCD.dir = 0;
        xLCD.width = LCD_WIDTH;
        xLCD.height = LED_HEIGHT;
    }
    else                              // 横屏
    {
        xLCD.dir = 1;
        xLCD.width = LED_HEIGHT;
        xLCD.height = LCD_WIDTH;
    }

    if (dir == 0) regval |= (0 << 7) | (0 << 6) | (0 << 5);
    if (dir == 3) regval |= (1 << 7) | (1 << 6) | (0 << 5);
    if (dir == 5) regval |= (0 << 7) | (1 << 6) | (1 << 5);
    if (dir == 6) regval |= (1 << 7) | (0 << 6) | (1 << 5);

    regval |= 0X08;
    LCD->LCD_REG = 0X36;
    LCD->LCD_RAM = regval;

    if (regval & 0X20)
    {
        if (xLCD.width < xLCD.height)
        {
            temp = xLCD.width;
            xLCD.width = xLCD.height;
            xLCD.height = temp;
        }
    }
    else
    {
        if (xLCD.width > xLCD.height)
        {
            temp = xLCD.width;
            xLCD.width = xLCD.height;
            xLCD.height = temp;
        }
    }

    LCD->LCD_REG = 0X2A;
    LCD->LCD_RAM = 0;
    LCD->LCD_RAM = 0;
    LCD->LCD_RAM = (xLCD.width - 1) >> 8;
    LCD->LCD_RAM = (xLCD.width - 1) & 0XFF;
    LCD->LCD_REG = 0X2B;
    LCD->LCD_RAM = 0;
    LCD->LCD_RAM = 0;
    LCD->LCD_RAM = (xLCD.height - 1) >> 8;
    LCD->LCD_RAM = (xLCD.height - 1) & 0XFF;
}

/******************************************************************
 * 函数名： LCD_Fill
 * 功  能： 在指定区域内填充单个颜色
 *****************************************************************/
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    uint16_t xlen = ex - sx + 1;
    for (uint16_t i = sy; i <= ey; i++)
    {
        setCursor(sx, i);
        LCD->LCD_REG = 0X2C;
        for (uint16_t j = 0; j < xlen; j++)
            LCD->LCD_RAM = color;
    }
}

/******************************************************************
 * 函数名： LCD_Line
 * 功  能： 画直线（Bresenham算法）
 *****************************************************************/
void LCD_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t _color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0) incx = 1;
    else if (delta_x == 0) incx = 0;
    else { incx = -1; delta_x = -delta_x; }
    if (delta_y > 0) incy = 1;
    else if (delta_y == 0) incy = 0;
    else { incy = -1; delta_y = -delta_y; }
    if (delta_x > delta_y) distance = delta_x;
    else distance = delta_y;
    for (t = 0; t <= distance + 1; t++)
    {
        LCD_DrawPoint(uRow, uCol, _color);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) { xerr -= distance; uRow += incx; }
        if (yerr > distance) { yerr -= distance; uCol += incy; }
    }
}

/******************************************************************
 * 函数名： LCD_Circle
 * 功  能： 在指定位置画圆
 *****************************************************************/
void LCD_Circle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint16_t _color)
{
    int16_t mx = Xpos, my = Ypos, x = 0, y = Radius;
    int16_t d = 1 - Radius;
    while (y > x)
    {
        LCD_DrawPoint(x + mx, y + my, _color);
        LCD_DrawPoint(-x + mx, y + my, _color);
        LCD_DrawPoint(-x + mx, -y + my, _color);
        LCD_DrawPoint(x + mx, -y + my, _color);
        LCD_DrawPoint(y + mx, x + my, _color);
        LCD_DrawPoint(-y + mx, x + my, _color);
        LCD_DrawPoint(y + mx, -x + my, _color);
        LCD_DrawPoint(-y + mx, -x + my, _color);
        if (d < 0) { d += 2 * x + 3; }
        else { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

/******************************************************************
 * 函数名： drawAscii
 * 功  能： 在指定位置显示一个 ASCII 字符
 *****************************************************************/
static void drawAscii(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint32_t bColor, uint32_t fColor)
{
    uint8_t temp;
    uint8_t csize;
    uint16_t y0 = y;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);
    num = num - ' ';   // ASCII字库从空格开始取模

    for (uint8_t t = 0; t < csize; t++)
    {
        if (size == 12)       temp = aFontASCII12[num][t];
        else if (size == 16)  temp = aFontASCII16[num][t];
        else if (size == 24)  temp = aFontASCII24[num][t];
        else if (size == 32)  temp = aFontASCII32[num][t];
        else                  return;

        for (uint8_t t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80) LCD_DrawPoint(x, y, fColor);
            else             LCD_DrawPoint(x, y, bColor);

            temp <<= 1;
            y++;
            if (y >= xLCD.height) return;
            if ((y - y0) == size)
            {
                y = y0;
                x++;
                if (x >= xLCD.width) return;
                break;
            }
        }
    }
}

/******************************************************************************
 * 函  数： LCD_String
 * 功  能： 在LCD上显示字符串（仅支持ASCII英文，不含汉字）
 * 说  明： 移除 W25Q128 外部字库依赖
 ******************************************************************************/
void LCD_String(uint16_t x, uint16_t y, char *pFont, uint8_t size, uint32_t fColor, uint32_t bColor)
{
    if (xLCD.FlagInit == 0) return;

    uint16_t xStart = x;

    if (size != 12 && size != 16 && size != 24 && size != 32)
        size = 24;

    while (*pFont != 0)
    {
        if (x > (xLCD.width - size))
        {
            x = xStart;
            y = y + size;
        }
        if (y > (xLCD.height - size))
            return;

        if (*pFont < 127)   // ASCII字符
        {
            drawAscii(x, y, (uint8_t)*pFont, size, bColor, fColor);
            pFont++;
            x += size / 2;
        }
        else                // 非ASCII字符（汉字等），跳过（无外部字库）
        {
            pFont += 2;
            x += size;
        }
    }
}

/******************************************************************
 * 函数名： LCD_Image
 * 功  能： 在指定区域内填充指定图片数据
 *****************************************************************/
void LCD_Image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *image)
{
    for (uint16_t i = 0; i < height; i++)
    {
        LCD->LCD_REG = 0X2A;
        LCD->LCD_RAM = x >> 8;
        LCD->LCD_RAM = x;
        LCD->LCD_REG = 0X2B;
        LCD->LCD_RAM = (y + i) >> 8;
        LCD->LCD_RAM = y + i;
        LCD->LCD_REG = 0X2C;
        for (uint16_t j = 0; j < width; j++)
        {
            LCD->LCD_RAM = image[1] << 8 | *image;
            image += 2;
        }
    }
}

/******************************************************************
 * 函数名： LCD_DispFlush
 * 功  能： 在指定区域内填充指定数据（高位在前，适用于LVGL）
 *****************************************************************/
void LCD_DispFlush(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pData)
{
    for (uint16_t nowY = y; nowY <= height; nowY++)
    {
        LCD->LCD_REG = 0X2A;
        LCD->LCD_RAM = x >> 8;
        LCD->LCD_RAM = x;
        LCD->LCD_REG = 0X2B;
        LCD->LCD_RAM = nowY >> 8;
        LCD->LCD_RAM = nowY;
        LCD->LCD_REG = 0X2C;
        for (uint16_t nowX = x; nowX <= width; nowX++)
        {
            LCD->LCD_RAM = *pData++;
        }
    }
}

/******************************************************************
 * 函数名： LCD_ShowChinese
 * 功  能： 显示自行取模的汉字（font.h 中的内嵌字模）
 *****************************************************************/
void LCD_ShowChinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1, uint32_t fColor, uint32_t bColor)
{
    uint8_t m, temp;
    uint8_t x0 = x, y0 = y;
    uint16_t size3 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * size1;

    for (uint16_t i = 0; i < size3; i++)
    {
        if (size1 == 12)        temp = aFontChinese12[num][i];
        else if (size1 == 16)   temp = aFontChinese16[num][i];
        else if (size1 == 24)   temp = aFontChinese24[num][i];
        else if (size1 == 32)   temp = aFontChinese32[num][i];
        else                    temp = aFontChinese12[num][i];

        for (m = 0; m < 8; m++)
        {
            if (temp & 0x01) LCD_DrawPoint(x, y, fColor);
            else             LCD_DrawPoint(x, y, bColor);
            temp >>= 1;
            y++;
        }
        x++;
        if ((x - x0) == size1) { x = x0; y0 = y0 + 8; }
        y = y0;
    }
}

/******************************************************************
 * 函数名： LCD_Cross
 * 功  能： 在指定点上绘制十字线，用于校准触摸屏
 *****************************************************************/
void LCD_Cross(uint16_t x, uint16_t y, uint16_t len, uint32_t fColor)
{
    LCD_Line(x - len, y, x + len, y, fColor);
    LCD_Line(x, y - len, x, y + len, fColor);
}
