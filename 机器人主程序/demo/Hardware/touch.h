/**
  ******************************************************************************
  * @file    touch.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   触摸传感器模块头文件
  ******************************************************************************
  */
#ifndef __TOUCH_H
#define __TOUCH_H

#include "stm32f10x.h"
#include "face.h"

// 函数声明
void Touch_Init(void);
uint8_t Get_PB10_State(void);
void Touch_Process(void);
uint8_t Is_Touching(void);
uint8_t Is_Allow_Other_Actions(void);

// 触摸回调函数类型定义
typedef void (*TouchCallback)(void);

// 注册回调函数
void Touch_Register_Start_Callback(TouchCallback callback);
void Touch_Register_End_Callback(TouchCallback callback);

#endif
