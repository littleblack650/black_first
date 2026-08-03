/**
  ******************************************************************************
  * @file    system_manager.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   系统管理器头文件
  ******************************************************************************
  */

#ifndef _SYSTEM_MANAGER_H_
#define _SYSTEM_MANAGER_H_

#include "stm32f10x.h"
#include "face.h"

/* 系统状态枚举 */
typedef enum {
    SYSTEM_STATE_INIT = 0,      // 初始化状态
    SYSTEM_STATE_READY,         // 就绪状态
    SYSTEM_STATE_WAKING,        // 唤醒中
    SYSTEM_STATE_RUNNING,       // 运行状态
    SYSTEM_STATE_STOPPED,       // 停止状态
    SYSTEM_STATE_ERROR          // 错误状态
} SystemState;

/* 函数声明 */
void SystemManager_Init(void);
void SystemManager_Start(void);
void SystemManager_Run(void);
void SystemManager_UpdateSpecialCases(void);
SystemState SystemManager_GetState(void);
void SystemManager_EmergencyStop(void);
void SystemManager_Resume(void);
void Task_Battery_Update(void);

#endif
