#ifndef __DICM_H
#define __DICM_H

#include "stm32f4xx.h"
#include "main.h"


extern uint8_t line_buf[2][LINE_SIZE];
extern volatile uint8_t dma_buf_idx;
extern volatile uint8_t line_ready;
extern volatile uint16_t line_idx;

void DCMI_Config(void);
void DCMI_DMA_Config(void);
void DCMI_Start(void);

#endif


