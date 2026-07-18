/**
  ******************************************************************************
  * @file    main.c
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   主函数
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "system_manager.h"
#include "delay.h"

/* 显示缓冲区 */
u8 readBuffer2[1024] __attribute__((section(".ccmram")));

void IWDG_Init(void)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);      // 64分频
    IWDG_SetReload(0xFFF);                     // 约1秒超时（72MHz/64≈1.125MHz，4096个周期约3.6ms？实际计算：40kHz低速时钟，64分频后625Hz，重载4095约6.5秒）请根据需求调整
    IWDG_ReloadCounter();
    IWDG_Enable();
}

/**
  * @brief  主函数
  */
int main(void)
{
    // 系统初始化
    SystemManager_Init();                                                                                                                                
    
	IWDG_Init();   // 启动看门狗
	
    // 启动系统
    SystemManager_Start();
    
    // 主循环
    while (1) {
        SystemManager_Run();
		IWDG_ReloadCounter();   // 喂狗
        Delay_ms(1);  // 给系统一些空闲时间
    }
}

/**
  * @brief  断言失败处理函数
  */
#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
    /* 用户可以添加自己的断言失败处理代码 */
    while (1)
    {
    }
}
#endif
