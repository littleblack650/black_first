/**
  ******************************************************************************
  * @file    sleep_mode.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   休眠模式管理模块头文件
  ******************************************************************************
  */
  
#ifndef _SLEEP_MODE_H_
#define _SLEEP_MODE_H_

#include "stm32f10x.h"

/* 休眠状态枚举 */
typedef enum {
    SLEEP_STATE_AWAKE = 0,      // 唤醒状态
    SLEEP_STATE_SLEEPING        // 休眠状态
} SleepState;

/* 函数声明 */
void SleepMode_Enter(void);
void SleepMode_Exit(void);
void SleepMode_Update(void);
SleepState SleepMode_GetState(void);
uint8_t SleepMode_IsSleeping(void);
void SleepMode_ForceWake(void);
uint32_t SleepMode_GetSleepDuration(void);

#endif
