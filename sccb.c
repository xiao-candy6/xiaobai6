
#include "main.h"
#include "sccb.h"
#include "Delay.h"

// 引脚操作宏（沿用原定义）
#define SCCB_SCL_H()   GPIO_SetBits(GPIOB, OV7670_SCL_PIN)
#define SCCB_SCL_L()   GPIO_ResetBits(GPIOB, OV7670_SCL_PIN)
#define SCCB_SDA_H()   GPIO_SetBits(GPIOB, OV7670_SDA_PIN)
#define SCCB_SDA_L()   GPIO_ResetBits(GPIOB, OV7670_SDA_PIN)
#define SCCB_SDA_READ() GPIO_ReadInputDataBit(GPIOB, OV7670_SDA_PIN)

#define OV7670_ADDR_WR 0x42
#define OV7670_ADDR_RD 0x43

#define SCCB_DELAY_US  2       // 半周期延时，与可运行代码一致

// ==================== 底层时序函数 ====================
static void SCCB_Start(void)
{
    SCCB_SDA_H();
    SCCB_SCL_H();
    Delay_us(SCCB_DELAY_US);
    SCCB_SDA_L();
    Delay_us(SCCB_DELAY_US);
    SCCB_SCL_L();
    Delay_us(SCCB_DELAY_US);
}

static void SCCB_Stop(void)
{
    SCCB_SDA_L();
    SCCB_SCL_H();
    Delay_us(SCCB_DELAY_US);
    SCCB_SDA_H();
    Delay_us(SCCB_DELAY_US);
}

// 写一个字节，返回应答（0=ACK, 1=NACK）
static uint8_t SCCB_WriteByte(uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (data & 0x80)
            SCCB_SDA_H();
        else
            SCCB_SDA_L();
        Delay_us(1);            // 数据建立时间
        SCCB_SCL_H();
        Delay_us(SCCB_DELAY_US);
        SCCB_SCL_L();
        Delay_us(1);
        data <<= 1;
    }
    // 释放总线，读应答
    SCCB_SDA_H();
    Delay_us(1);
    SCCB_SCL_H();
    Delay_us(SCCB_DELAY_US);
    uint8_t ack = SCCB_SDA_READ();   // 0=ACK, 1=NACK
    SCCB_SCL_L();
    Delay_us(1);
    return ack;
}

// 读一个字节，ack=0时主机发送应答（继续读），ack=1时主机发送非应答（结束读）
static uint8_t SCCB_ReadByte(uint8_t ack)
{
    uint8_t i, data = 0;
    SCCB_SDA_H();               // 释放总线
    Delay_us(1);
    for (i = 0; i < 8; i++) {
        data <<= 1;
        SCCB_SCL_H();
        Delay_us(SCCB_DELAY_US);
        if (SCCB_SDA_READ())
            data |= 0x01;
        SCCB_SCL_L();
        Delay_us(1);
    }
    // 主机发送应答/非应答
    if (ack)
        SCCB_SDA_H();           // 非应答
    else
        SCCB_SDA_L();           // 应答
    Delay_us(1);
    SCCB_SCL_H();
    Delay_us(SCCB_DELAY_US);
    SCCB_SCL_L();
    Delay_us(1);
    SCCB_SDA_H();               // 释放总线
    return data;
}

// ==================== 对外接口 ====================
void SCCB_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    // 使能 GPIOB 时钟
   // RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    // PB10 SCL, PB11 SDA 开漏输出，无需内部上拉（外部上拉）
    GPIO_InitStruct.GPIO_Pin   = OV7670_SCL_PIN | OV7670_SDA_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    // 总线空闲状态
    SCCB_SCL_H();
    SCCB_SDA_H();
}

uint8_t SCCB_Write_Byte(uint8_t reg, uint8_t data)
{
    uint8_t retry = 0;
    while (retry < 3) {
        SCCB_Start();
        if (SCCB_WriteByte(OV7670_ADDR_WR) == 0) { // 设备地址 ACK
            if (SCCB_WriteByte(reg) == 0) {        // 寄存器地址 ACK
                if (SCCB_WriteByte(data) == 0) {   // 数据 ACK
                    SCCB_Stop();
                    return 1;                      // 成功
                }
            }
        }
        SCCB_Stop();
        retry++;
        Delay_us(1000);
    }
    return 0;  // 失败
}

uint8_t SCCB_Read_Byte(uint8_t reg, uint8_t *data)
{
    uint8_t retry = 0;
    while (retry < 3) {
        // 第一阶段：写寄存器地址
        SCCB_Start();
        if (SCCB_WriteByte(OV7670_ADDR_WR) == 0) {
            if (SCCB_WriteByte(reg) == 0) {
                SCCB_Stop();
                Delay_us(10);                     // 等待器件准备
                // 第二阶段：读数据
                SCCB_Start();
                if (SCCB_WriteByte(OV7670_ADDR_RD) == 0) {
                    *data = SCCB_ReadByte(1);     // 读一个字节，主机发送非应答
                    SCCB_Stop();
                    return 1;                     // 成功
                }
            }
        }
        SCCB_Stop();
        retry++;
        Delay_us(1000);
    }
    return 0;  // 失败
}