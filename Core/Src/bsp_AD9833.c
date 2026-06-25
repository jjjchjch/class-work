#include "bsp_AD9833.h"

#define AD9833_CS_HIGH     HAL_GPIO_WritePin(AD9833_CS_GPIO, AD9833_CS_PIN, GPIO_PIN_SET)
#define AD9833_CS_LOW      HAL_GPIO_WritePin(AD9833_CS_GPIO, AD9833_CS_PIN, GPIO_PIN_RESET)

#define AD9833_SCK_HIGH    HAL_GPIO_WritePin(AD9833_SCK_GPIO, AD9833_SCK_PIN, GPIO_PIN_SET)
#define AD9833_SCK_LOW     HAL_GPIO_WritePin(AD9833_SCK_GPIO, AD9833_SCK_PIN, GPIO_PIN_RESET)

#define AD9833_DATA_HIGH   HAL_GPIO_WritePin(AD9833_DATA_GPIO, AD9833_DATA_PIN, GPIO_PIN_SET)
#define AD9833_DATA_LOW    HAL_GPIO_WritePin(AD9833_DATA_GPIO, AD9833_DATA_PIN, GPIO_PIN_RESET)

static void SPI_Delay(uint32_t count)
{
    uint32_t i;
    for (i = 0; i < count; i++)
    {
        uint8_t uc = 12;
        while (uc--);
    }
}

/******************************************************************************
 * 函  数： AD9833_Init
 * 功  能： 初始化 AD9833 所需引脚 (软件模拟 SPI)
 * 参  数： 无
 * 返回值： 无
 ******************************************************************************/
void AD9833_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStructure.Pin   = AD9833_CS_PIN | AD9833_SCK_PIN | AD9833_DATA_PIN;
    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull  = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    AD9833_CS_HIGH;                    /* CS 拉高: 停止信号 */
}

/******************************************************************************
 * 函 数 名: AD9833_WriteData
 * 功能说明: AD9833 写入 16 位数据
 * 形    参: TxData 待写入的 16 位数据
 * 返 回 值: 无
 ******************************************************************************/
void AD9833_WriteData(uint16_t TxData)
{
    int i;
    AD9833_SCK_HIGH;
    AD9833_CS_HIGH;
    AD9833_CS_LOW;
    /* 写 16 位数据 */
    for (i = 0; i < 16; i++)
    {
        if (TxData & 0x8000)
            AD9833_DATA_HIGH;
        else
            AD9833_DATA_LOW;
        SPI_Delay(10);
        AD9833_SCK_LOW;
        SPI_Delay(10);
        AD9833_SCK_HIGH;
        TxData <<= 1;
    }
    AD9833_CS_HIGH;                    /* 发送完毕, 拉高片选 */
}

/**************************************
 * 函 数 名: AD9833_SetFrequency
 * 功能说明: AD9833 设置频率寄存器
 * 形    参: reg 待写入的频率寄存器
 *           fout 频率值
 * 返 回 值: 无
 *************************************/
void AD9833_SetFrequency(unsigned short reg, double fout)
{
    int frequence_LSB, frequence_MSB;
    double   frequence_mid, frequence_DATA;
    long int frequence_hex;

    /*********************************计算频率的16进制值***********************************/
    frequence_mid = 268435456 / 25;       /* 适合 25M 晶振, 2^28 */
    /* 如果时钟频率不为 25MHz, 修改该处的频率值, 单位 MHz, AD9833 最大支持 25MHz */
    frequence_DATA = fout;
    frequence_DATA = frequence_DATA / 1000000;
    frequence_DATA = frequence_DATA * frequence_mid;
    frequence_hex  = frequence_DATA;      /* 拆分成两个 14 位 */
    frequence_LSB  = frequence_hex;        /* 低 16 位 */
    frequence_LSB  = frequence_LSB & 0x3fff;
    frequence_MSB  = frequence_hex >> 14;  /* 高 16 位 */
    frequence_MSB  = frequence_MSB & 0x3fff;
    frequence_LSB  = frequence_LSB | reg;
    frequence_MSB  = frequence_MSB | reg;
    AD9833_WriteData(0x2100);             /* B28 + RESET = 1 */
    AD9833_WriteData(frequence_LSB);
    AD9833_WriteData(frequence_MSB);
}

/**************************************
 * 函 数 名: AD9833_SetPhase
 * 功能说明: AD9833 设置相位寄存器
 * 形    参: reg 待写入的相位寄存器
 *           val 相位值
 * 返 回 值: 无
 *************************************/
void AD9833_SetPhase(unsigned short reg, unsigned short val)
{
    unsigned short phase = reg;
    phase |= val;
    AD9833_WriteData(phase);
}

/**************************************
 * 函 数 名: AD9833_SetWave
 * 功能说明: AD9833 设置波形
 * 形    参: WaveMode 输出波形类型
 *           Freq_SFR 输出的频率寄存器类型
 *           Phase_SFR 输出的相位寄存器类型
 * 返 回 值: 无
 *************************************/
void AD9833_SetWave(unsigned int WaveMode, unsigned int Freq_SFR, unsigned int Phase_SFR)
{
    unsigned int val = 0;
    val = (val | WaveMode | Freq_SFR | Phase_SFR);
    AD9833_WriteData(val);
}

/**************************************
 * 函 数 名: AD9833_Setup
 * 功能说明: AD9833 设置输出
 * 形    参: Freq_SFR 频率寄存器类型
 *           Freq 频率值
 *           Phase_SFR 相位寄存器类型
 *           Phase 相位值
 *           WaveMode 波形类型
 * 返 回 值: 无
 *************************************/
void AD9833_Setup(unsigned int Freq_SFR, double Freq, unsigned int Phase_SFR,
                  unsigned int Phase, unsigned int WaveMode)
{
    unsigned int Fsel, Psel;
    AD9833_WriteData(0x0100);             /* 复位 AD9833 (RESET=1) */
    AD9833_WriteData(0x2100);             /* B28 + RESET = 1 */
    AD9833_SetFrequency(Freq_SFR, Freq);
    AD9833_SetPhase(Phase_SFR, Phase);
    if (Freq_SFR == AD9833_REG_FREQ0)
        Fsel = AD9833_FSEL0;
    else
        Fsel = AD9833_FSEL1;
    if (Phase_SFR == AD9833_REG_PHASE0)
        Psel = AD9833_PSEL0;
    else
        Psel = AD9833_PSEL1;

    AD9833_SetWave(WaveMode, Fsel, Psel);
}
