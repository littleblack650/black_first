/**
  ******************************************************************************
  * @file    sleep_mode.c
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   休眠模式管理模块源文件
  ******************************************************************************
  */

#include "sleep_mode.h"

void SleepMode_Enter(void)
{
    // 无操作（休眠功能已禁用）
}

void SleepMode_Exit(void)
{
    // 无操作
}

void SleepMode_Update(void)
{
    // 无操作
}

SleepState SleepMode_GetState(void)
{
    return SLEEP_STATE_AWAKE;
}

uint8_t SleepMode_IsSleeping(void)
{
    return 0;
}

void SleepMode_ForceWake(void)
{
    // 无操作
}

uint32_t SleepMode_GetSleepDuration(void)
{
    return 0;
}
