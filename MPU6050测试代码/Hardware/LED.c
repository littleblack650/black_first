#include "led.h"

/**
  * @brief  初始化板载LED（PC13）
  */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. 使能GPIOC时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // 2. 配置PC13为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  // 2MHz输出速度
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 3. 初始状态关闭LED
    GPIO_SetBits(GPIOC, GPIO_Pin_13);  // PC13高电平关闭LED
}

/**
  * @brief  切换LED状态
  */
void LED_Toggle(void)
{
    GPIOC->ODR ^= GPIO_Pin_13;  // 使用异或操作翻转引脚状态
}

/**
  * @brief  打开LED
  */
void LED_On(void)
{
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);  // PC13低电平打开LED
}

/**
  * @brief  关闭LED
  */
void LED_Off(void)
{
    GPIO_SetBits(GPIOC, GPIO_Pin_13);  // PC13高电平关闭LED
}
