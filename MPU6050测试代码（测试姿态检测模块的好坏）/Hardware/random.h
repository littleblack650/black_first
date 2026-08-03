#ifndef __RANDOM_H
#define __RANDOM_H

#include <stdint.h>

/**
  * @brief  获取随机地址
  * @retval 随机选择的地址值 (0x000000, 0x004000, 0x005000)
  */
uint32_t Get_Random_Address(void);//随机输出三个地址中的一个

#endif
