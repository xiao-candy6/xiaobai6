#include "stm32f4xx.h"

#ifndef __ST7735_H
#define __ST7735_H


void ST7735_Init(void);
void ST7735_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void ST7735_WriteColor(uint16_t color, uint16_t len);
void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void ST7735_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color);



void ST7735_DrawLineImage(uint16_t y, const uint8_t *lineBuf);
void ST7735_PutString(uint16_t x, uint16_t y, const char *str,uint16_t color, uint16_t bg_color);
void ST7735_DrawChar_8x8(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg);
void ST7735_DrawString_8x8(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);



/* ========== TFT 控制宏 ========== */
/* TFT 控制宏 */
#define TFT_SCL(x)   GPIO_WriteBit(TFT_SCL_PORT, TFT_SCL_PIN, (BitAction)(x))
#define TFT_SDA(x)   GPIO_WriteBit(TFT_SDA_PORT, TFT_SDA_PIN, (BitAction)(x))
#define TFT_CS(x)    GPIO_WriteBit(TFT_CS_PORT,  TFT_CS_PIN,  (BitAction)(x))
#define TFT_DC(x)    GPIO_WriteBit(TFT_DC_PORT,  TFT_DC_PIN,  (BitAction)(x))
#define TFT_RST(x)   GPIO_WriteBit(TFT_RST_PORT, TFT_RST_PIN, (BitAction)(x))
/* ========== TFT 控制引脚（新：软件SPI） ========== */
#define TFT_SCL_PORT      GPIOD
#define TFT_SCL_PIN       GPIO_Pin_2    // PD2 时钟

#define TFT_SDA_PORT      GPIOD
#define TFT_SDA_PIN       GPIO_Pin_3    // PD3 数据（MOSI）

#define TFT_CS_PORT       GPIOD
#define TFT_CS_PIN        GPIO_Pin_4    // PD4 片选

#define TFT_DC_PORT       GPIOE
#define TFT_DC_PIN        GPIO_Pin_1    // PE1 命令/数据选择

#define TFT_RST_PORT      GPIOE
#define TFT_RST_PIN       GPIO_Pin_2    // PE2 复位


#endif
