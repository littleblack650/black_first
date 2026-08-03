/**
  ******************************************************************************
  * @file    LED.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   板载LED辅助测试模块头文件
  ******************************************************************************
  */
  
#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

/**
  * @brief  初始化板载LED（PC13）
  */
void LED_Init(void);

/**
  * @brief  切换LED状态
  */
void LED_Toggle(void);

/**
  * @brief  打开LED
  */
void LED_On(void);

/**
  * @brief  关闭LED
  */
void LED_Off(void);

#endif /* __LED_H */
