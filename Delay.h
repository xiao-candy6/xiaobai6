//#ifndef __DELAY_H
//#define __DELAY_H

//#include "stm32f4xx.h"

//void Delay_Init(void);
//void Delay_us(uint32_t us);
//void Delay_ms(uint32_t ms);

//#endif /* __DELAY_H 
#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"

//void Delay_Init(void);          // 初始化 SysTick，1ms 中断一次
void Delay_ms(uint32_t ms);     // 毫秒延时
void Delay_us(uint32_t us);  // 微秒延时（忙等，不依赖中断）
void Delay_s(uint32_t s);
//void SysTick_Inc(void); 
#endif
