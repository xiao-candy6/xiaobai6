#include "stm32f4xx.h"

/**
  * @brief  微秒级延时（基于SysTick）
  * @param  us 延时时长，单位微秒（最大连续延时约99ms，超出会自动拆分）
  * @retval 无
  */
void Delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t max_ticks = 0xFFFFFF;  // SysTick为24位计数器，最大值16777215

    // 计算所需SysTick时钟周期数（HCLK频率 / 1MHz * us）
    ticks = (SystemCoreClock / 1000000) * us;

    while (ticks > max_ticks)
    {
        // 单次延时超过最大值时，分多次执行
        SysTick->LOAD = max_ticks;
        SysTick->VAL = 0;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
        ticks -= max_ticks;
    }

    // 最后一次（或唯一一次）延时
    SysTick->LOAD = ticks;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;  // 关闭定时器
}

/**
  * @brief  毫秒级延时
  * @param  ms 延时时长，单位毫秒（范围0~4294967295）
  * @retval 无
  */
void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}

/**
  * @brief  秒级延时
  * @param  s 延时时长，单位秒（范围0~4294967295）
  * @retval 无
  */
void Delay_s(uint32_t s)
{
    while (s--)
    {
        Delay_ms(1000);
    }
}