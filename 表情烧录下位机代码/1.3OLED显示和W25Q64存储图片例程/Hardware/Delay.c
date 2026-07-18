#include "delay.h"

__IO uint32_t ulTicks;

static uint8_t delay_initialized_flag = 0;
static float us_per_mini_tick;


//
// @简介：初始化延迟函数
// @返回值：无
//
void Delay_Init(void)
{
	if(!delay_initialized_flag)
	{
		delay_initialized_flag = 1;
		
		RCC_ClocksTypeDef clockinfo = {0};
		uint32_t tmp;
		
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE; // 禁止SYSTICK

		ulTicks = 0;

		RCC_GetClocksFreq(&clockinfo);

		SysTick->CTRL |= SysTick_CTRL_TICKINT; // 开启中断

		// 设置中断优先级为0
		SCB->SHP[7] = 0;

		// 设置自动重装值以保证1ms的时钟
		tmp =  clockinfo.HCLK_Frequency / 1000;
		if(tmp > 0x00ffffff)
		{
			tmp = tmp / 8;
			SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE; 
		}
		else
		{
			SysTick->CTRL |= SysTick_CTRL_CLKSOURCE; 
		}
		SysTick->LOAD = tmp - 1;

		SysTick->CTRL |= SysTick_CTRL_ENABLE; 
		
		us_per_mini_tick = 1000.0 / ((SysTick->LOAD & 0x00ffffff) + 1);
	}
}

//
// @简介：毫秒级延迟
// @参数：Delay - 延迟时长，以毫秒为单位(千分之一秒)
// @返回值：无
// @注意：不允许在中断响应函数中调用此方法
//
void Delay_ms(uint32_t Delay)
{
	Delay_Init();
	
	uint64_t expiredTime = ulTicks + Delay;

	while(ulTicks <  expiredTime){}
}

//
// @简介：获取当前时间，以毫秒（千分之一秒）为单位
// @参数：无
// @返回值：当前时间，单位为毫秒（千分之一秒）
//
uint32_t GetTick(void)
{
	Delay_Init();
	
	return ulTicks;
}

//
// @简介：获取当前的微秒级时间
// @返回值：当前的微秒级时间
//
uint64_t GetUs(void)
{
	Delay_Init();
	
	uint64_t tick;
	uint32_t mini_tick;
	
	
	// 直到无溢出标志
	// 读取COUNTERFLAG也会清除它的值
	
	__disable_irq();
	
	while(1)
	{
		tick = ulTicks; // 读取毫秒值
		mini_tick = SysTick->VAL; // 读取SYSTICK的值
		
		if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG)
		{
			ulTicks++;
		}
		else
		{
			break;
		}
	}
	
	__enable_irq();
	
	// 换算成微秒
	tick *= 1000; // 毫秒部分乘以1000
	tick += (uint32_t)((SysTick->LOAD - mini_tick) * us_per_mini_tick); // 小数部分折算成微秒
	
	return tick;
}

//
// @简介：微秒级延迟
// @参数：us - 要延迟的时间，单位是微秒
//
void Delay_us(uint32_t us)
{
	Delay_Init();
	
	uint64_t expiredTime = GetUs() + us + 1;
	
	while(GetUs() < expiredTime);
}

