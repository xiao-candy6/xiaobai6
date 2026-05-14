#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>

/* ========== OV7670 数据引脚 ========== */
#define OV7670_D0_PIN    GPIO_Pin_6    // PC6
#define OV7670_D1_PIN    GPIO_Pin_7    // PC7
#define OV7670_D2_PIN    GPIO_Pin_8    // PC8
#define OV7670_D3_PIN    GPIO_Pin_9    // PC9
#define OV7670_D4_PIN    GPIO_Pin_11   // PC11
#define OV7670_D5_PIN    GPIO_Pin_6    // PB6
#define OV7670_D6_PIN    GPIO_Pin_8    // PB8
#define OV7670_D7_PIN    GPIO_Pin_9    // PB9

/* ========== OV7670 同步与控制引脚 ========== */
#define OV7670_PCLK_PIN   GPIO_Pin_6    // PA6
#define OV7670_HREF_PIN   GPIO_Pin_4    // PA4
#define OV7670_VSYNC_PIN  GPIO_Pin_7    // PB7
#define OV7670_XCLK_PIN   GPIO_Pin_8    // PA8
#define OV7670_RST_PIN    GPIO_Pin_0    // PD0
#define OV7670_PWDN_PIN   GPIO_Pin_10   // PC10

/* ========== SCCB 引脚 ========== */
#define OV7670_SCL_PIN    GPIO_Pin_10   // PB10
#define OV7670_SDA_PIN    GPIO_Pin_11   // PB11


/* ========== 帧同步信号引脚 ========== */
#define FRAME_SYNC_PORT   GPIOE
#define FRAME_SYNC_PIN    GPIO_Pin_0    // PE0

/* ========== 图像参数 ========== */
#define IMAGE_WIDTH       160
#define IMAGE_HEIGHT      128
#define LINE_SIZE         (IMAGE_WIDTH * 2)   // RGB565 每个像素2字节 = 640 字节/行



/* ========== 函数声明 ========== */
void SystemClock_Config(void);
void GPIO_Config(void);

/* OV7670 */
void OV7670_XCLK_Config(void);
uint8_t OV7670_Init(void);

/* SCCB */
void SCCB_Init(void);
uint8_t SCCB_Write_Byte(uint8_t reg, uint8_t data);
uint8_t SCCB_Read_Byte(uint8_t reg, uint8_t *data);

/* DCMI */
void DCMI_Config(void);
void DCMI_DMA_Config(void);
void DCMI_Start(void);


#endif /* __MAIN_H */
