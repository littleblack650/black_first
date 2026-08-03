#include "random.h"
#include "stm32f10x.h"

/**
  * @brief  增强随机性的初始化函数
  * @note   使用未初始化内存作为额外熵源
  */
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_adc.h"


// 最简单的伪随机数生成器（单行实现）
uint32_t simple_random(void)
{
    static uint32_t seed = 1;
    return (seed = (seed * 22695477 + 1)) % 3; // 直接返回0-2的随机索引
}

// 直接获取随机地址
uint32_t Get_Random_Address(void)
{
    static const uint32_t addresses[3] = {0x000000, 0x004000, 0x005000};
    return addresses[simple_random()];
}
