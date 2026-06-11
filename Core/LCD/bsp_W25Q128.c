/***********************************************************************************************************************************
 ** 【文件名称】  bsp_w25qxx.c
 ** 【编写人员】  魔女开发板团队
 ** 【淘    宝】  魔女开发板      https://demoboard.taobao.com
 ***********************************************************************************************************************************/
#include "bsp_W25Q128.h"

#define    W25Q80            0XEF13
#define    W25Q16            0XEF14
#define    W25Q32            0XEF15
#define    W25Q64            0XEF16
#define    W25Q128           0XEF17
#define    W25Q256           0XEF18
#define    W25Q128_CS_HIGH    (W25Q128_CS_GPIO -> BSRR =  W25Q128_CS_PIN)
#define    W25Q128_CS_LOW     (W25Q128_CS_GPIO -> BSRR =  W25Q128_CS_PIN << 16)

xW25Q_TypeDef  xW25Q128;

static uint8_t  sendByte(uint8_t d)
{
    uint16_t retry = 0;
    while ((W25Q128_SPI->SR & 2) == 0) { retry++; if (retry > 1000) return 0; }
    W25Q128_SPI->DR = d;
    retry = 0;
    while ((W25Q128_SPI->SR & 1) == 0) { retry++; if (retry > 1000) return 0; }
    return W25Q128_SPI->DR;
}

static void writeEnable()  { W25Q128_CS_LOW; sendByte(0x06); W25Q128_CS_HIGH; }

static void WaitReady()
{
    W25Q128_CS_LOW;
    sendByte(0x05);
    while (sendByte(0xFF) & 1) {}
    W25Q128_CS_HIGH;
}

static void eraseSector(uint32_t addr)
{
    if (xW25Q128.FlagInit == 0) return;
    addr *= 4096;
    writeEnable(); WaitReady();
    W25Q128_CS_LOW;
    sendByte(0x20); sendByte((uint8_t)(addr >> 16)); sendByte((uint8_t)(addr >> 8)); sendByte((uint8_t)addr);
    W25Q128_CS_HIGH;
    WaitReady();
}

static void writeSector(uint32_t addr, uint8_t *p, uint16_t num)
{
    if (xW25Q128.FlagInit == 0) return;
    uint16_t pageRemain = 256;
    for (char i = 0; i < 16; i++)
    {
        writeEnable(); WaitReady();
        W25Q128_CS_LOW;
        sendByte(0x02); sendByte((uint8_t)(addr >> 16)); sendByte((uint8_t)(addr >> 8)); sendByte((uint8_t)addr);
        for (uint16_t j = 0; j < pageRemain; j++) sendByte(p[j]);
        W25Q128_CS_HIGH;
        WaitReady();
        p += pageRemain; addr += pageRemain;
    }
}

static uint32_t readID(void)
{
    uint16_t Temp = 0;
    W25Q128_CS_LOW;
    sendByte(0x90); sendByte(0x00); sendByte(0x00); sendByte(0x00);
    Temp |= sendByte(0xFF) << 8; Temp |= sendByte(0xFF);
    W25Q128_CS_HIGH;
    xW25Q128.FlagInit = 1;
    switch (Temp)
    {
        case W25Q16:  sprintf((char *)xW25Q128.type, "%s", "W25Q16"); break;
        case W25Q32:  sprintf((char *)xW25Q128.type, "%s", "W25Q32"); break;
        case W25Q64:  sprintf((char *)xW25Q128.type, "%s", "W25Q64"); break;
        case W25Q128: sprintf((char *)xW25Q128.type, "%s", "W25Q128"); break;
        case W25Q256: sprintf((char *)xW25Q128.type, "%s", "W25Q256"); break;
        default:      sprintf((char *)xW25Q128.type, "%s", "Flash设备失败!!!"); xW25Q128.FlagInit = 0; break;
    }
    return Temp;
}

static void checkFlagGBKStorage(void)
{
    if (xW25Q128.FlagInit == 0) return;
    uint8_t sub = 0, f = 0;
    for (uint32_t i = 0; i < 6128640; i += 1000000) { W25Q128_ReadData(GBK_STORAGE_ADDR + i, &f, 1); sub += f; }
    xW25Q128.FlagGBKStorage = (sub == 146 ? 1 : 0);
}

uint8_t W25Q128_Init(void)
{
    if (W25Q128_CS_GPIO == GPIOA) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    if (W25Q128_CS_GPIO == GPIOB) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    if (W25Q128_CS_GPIO == GPIOC) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    if (W25Q128_CS_GPIO == GPIOD) RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    if (W25Q128_CS_GPIO == GPIOE) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    if (W25Q128_CS_GPIO == GPIOF) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    if (W25Q128_CS_GPIO == GPIOG) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;
    if (W25Q128_SCK_GPIO == GPIOA) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = W25Q128_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(W25Q128_CS_GPIO, &GPIO_InitStruct);
    W25Q128_CS_HIGH;

    GPIO_InitStruct.Pin   = W25Q128_SCK_PIN | W25Q128_MISO_PIN | W25Q128_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = W25Q128_SPI_AFx;
    HAL_GPIO_Init(W25Q128_SCK_GPIO, &GPIO_InitStruct);

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    W25Q128_SPI->CR1 = (0x1 << 0) | (0x1 << 1) | (0x1 << 2) | (0x0 << 3) | (0x0 << 7) | (0x1 << 9) | (0x1 << 8) | (0x0 << 11) | (0x1 << 6);

    readID();
    checkFlagGBKStorage();
    return xW25Q128.FlagInit ? 1 : 0;
}

void W25Q128_ReadData(uint32_t addr, uint8_t *pData, uint16_t num)
{
    if (xW25Q128.FlagInit == 0) return;
    W25Q128_CS_LOW;
    sendByte(0x03); sendByte((uint8_t)(addr >> 16)); sendByte((uint8_t)(addr >> 8)); sendByte((uint8_t)addr);
    for (uint32_t i = 0; i < num; i++) pData[i] = sendByte(0xFF);
    W25Q128_CS_HIGH;
}

uint8_t W25Q128_buffer[4096];

void W25Q128_WriteData(uint32_t addr, uint8_t *pData, uint16_t num)
{
    if (xW25Q128.FlagInit == 0) return;
    if (((addr + num) > 0x00A00000) && (xW25Q128.FlagGBKStorage == 1)) return;

    uint32_t secPos = addr / 4096;
    uint16_t secOff = addr % 4096;
    uint16_t secRemain = 4096 - secOff;
    uint8_t *buf = W25Q128_buffer;
    if (num <= secRemain) secRemain = num;
    while (1)
    {
        W25Q128_ReadData(secPos * 4096, buf, 4096);
        eraseSector(secPos);
        for (uint16_t i = 0; i < secRemain; i++) buf[secOff + i] = pData[i];
        writeSector(secPos * 4096, buf, 4096);
        if (secRemain == num) break;
        pData += secRemain; secPos++; secOff = 0; num -= secRemain;
        secRemain = (num > 4096) ? 4096 : num;
    }
}

void W25Q128_ReadFontData(uint8_t *pFont, uint8_t size, uint8_t *fontData)
{
    uint8_t qh, ql;
    uint32_t foffset;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);

    qh = *pFont; ql = *(++pFont);
    if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff)
    {
        for (uint8_t i = 0; i < csize; i++) *fontData++ = 0x00;
        return;
    }
    if (ql < 0x7f) ql -= 0x40; else ql -= 0x41;
    qh -= 0x81;
    foffset = ((unsigned long)190 * qh + ql) * csize;

    switch (size)
    {
        case 12: W25Q128_ReadData(foffset + GBK_STORAGE_ADDR,            fontData, csize); break;
        case 16: W25Q128_ReadData(foffset + GBK_STORAGE_ADDR + 0x0008c460, fontData, csize); break;
        case 24: W25Q128_ReadData(foffset + GBK_STORAGE_ADDR + 0x001474E0, fontData, csize); break;
        case 32: W25Q128_ReadData(foffset + GBK_STORAGE_ADDR + 0x002EC200, fontData, csize); break;
    }
}
