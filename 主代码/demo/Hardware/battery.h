/**
  ******************************************************************************
  * @file    battery.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   电池电量检测模块头文件
  ******************************************************************************
  */

#ifndef _BATTERY_H_
#define _BATTERY_H_

#include "stm32f10x.h"

/* 电池状态枚举 */
typedef enum {
    BATTERY_STATE_NORMAL = 0,    // 正常状态
    BATTERY_STATE_LOW,           // 低电量状态
    BATTERY_STATE_CHARGING       // 充电状态
} BatteryState;

/* 函数声明 */
void Battery_Init(void);
void Battery_UpdateVoltage(void);
void Battery_Update(void);
float Battery_GetVoltage(void);
BatteryState Battery_GetState(void);
uint8_t Battery_IsLow(void);
uint8_t Battery_IsCharging(void);
void Battery_ForceLowWarning(void);
void Battery_ForceNormal(void);

#endif
