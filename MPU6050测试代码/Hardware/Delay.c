#include "stm32f10x.h"
#include "Delay.h"

/**
  * @brief  微秒级延时（轮询方式，不依赖中断）
  * @param  xus 延时时长，范围：0~233015
  * @retval 无
  */
void Delay_us(uint32_t xus)
{
    SysTick->LOAD = 72 * xus;               // 72MHz时钟
    SysTick->VAL = 0x00;
    SysTick->CTRL = 0x00000005;             // 使能，使用HCLK
    while(!(SysTick->CTRL & 0x00010000));   // 等待计数到0
    SysTick->CTRL = 0x00000004;             // 关闭定时器
}

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长
  */
void Delay_ms(uint32_t xms)
{
    while(xms--)
    {
        Delay_us(1000);
    }
}

/**
  * @brief  秒级延时
  */
void Delay_s(uint32_t xs)
{
    while(xs--)
    {
        Delay_ms(1000);
    }
}

/**
  * @brief  初始化延时（空函数，保持接口一致）
  */
void Delay_Init(void)
{
    // 无需配置SysTick中断，仅使用轮询
}
