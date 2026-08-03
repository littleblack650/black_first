/**
  ******************************************************************************
  * @file    Delay.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   延时函数模块头文件
  ******************************************************************************
  */
#ifndef _DELAY_H_
#define _DELAY_H_

#include "stm32f10x.h"

    void Delay_Init(void); // 延迟函数初始化
    void Delay_ms(uint32_t ms); // 延迟
uint32_t GetTick(void); // 获取系统的当前时间
uint64_t GetUs(void); // 获取当前的微秒级时间
    void Delay_us(uint32_t us); // 微秒级延迟

#endif
