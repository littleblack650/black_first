/**
  ******************************************************************************
  * @file    task_scheduler.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   任务调度器头文件
  ******************************************************************************
  */

#ifndef _TASK_SCHEDULER_H_
#define _TASK_SCHEDULER_H_

#include "stm32f10x.h"
#include "system_config.h"

/* 最大任务数量 */
#define MAX_TASKS 25

/* 任务函数指针类型 */
typedef void (*TaskFunction)(void);

/* 任务状态结构体 */
typedef struct {
    uint32_t interval;
    uint8_t priority;
    uint8_t enabled;
    const char* name;
} TaskStatus;

/* 函数声明 */
void TaskScheduler_Init(void);
uint8_t TaskScheduler_Register(TaskFunction function, uint32_t interval, 
                              uint8_t priority, const char* name);
void TaskScheduler_SetEnable(uint8_t task_id, uint8_t enable);
void TaskScheduler_Run(void);
void TaskScheduler_GetStatus(TaskStatus* status, uint8_t* count);
void TaskScheduler_Start(void);
void TaskScheduler_Stop(void);
uint8_t TaskScheduler_GetTaskCount(void);

#endif
