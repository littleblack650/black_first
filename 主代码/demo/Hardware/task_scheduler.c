/**
  ******************************************************************************
  * @file    task_scheduler.c
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   任务调度器 - 统一管理系统任务调度
  ******************************************************************************
  */

#include "task_scheduler.h"
#include "system_config.h"
#include "delay.h"
#include <stddef.h>  // 标准头文件

/* 任务结构体定义 */
typedef struct {
    TaskFunction function;        // 任务函数指针
    uint32_t interval;           // 执行间隔(ms)
    uint32_t last_run_time;      // 上次执行时间
    uint8_t priority;            // 任务优先级
    uint8_t enabled;             // 任务使能标志
    const char* name;            // 任务名称(调试用)
} Task;

/* 任务列表 */
static Task task_list[MAX_TASKS];
static uint8_t task_count = 0;
static uint8_t scheduler_running = 0;

/**
  * @brief  初始化任务调度器
  */
void TaskScheduler_Init(void)
{
    task_count = 0;
    scheduler_running = 1;
    
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        task_list[i].function = NULL;
        task_list[i].enabled = 0;
    }
}

/**
  * @brief  注册新任务
  */
uint8_t TaskScheduler_Register(TaskFunction function, uint32_t interval, 
                              uint8_t priority, const char* name)
{
    if (task_count >= MAX_TASKS || function == NULL) {
        return 0;
    }
    
    task_list[task_count].function = function;
    task_list[task_count].interval = interval;
    task_list[task_count].priority = priority;
    task_list[task_count].last_run_time = GetTick();
    task_list[task_count].enabled = 1;
    task_list[task_count].name = name;
    
    task_count++;
    return 1;
}

/**
  * @brief  设置任务使能状态
  */
void TaskScheduler_SetEnable(uint8_t task_id, uint8_t enable)
{
    if (task_id < task_count) {
        task_list[task_id].enabled = enable;
    }
}

/**
  * @brief  任务调度器主循环
  */
void TaskScheduler_Run(void)
{
    if (!scheduler_running) {
        return;
    }
    
    uint32_t current_time = GetTick();
    
    // 按优先级执行任务
    for (uint8_t priority = TASK_PRIORITY_HIGH; priority <= TASK_PRIORITY_LOW; priority++) {
        for (uint8_t i = 0; i < task_count; i++) {
            if (task_list[i].enabled && 
                task_list[i].priority == priority &&
                (current_time - task_list[i].last_run_time) >= task_list[i].interval) {
                
                // 执行任务
                task_list[i].function();
                task_list[i].last_run_time = current_time;
            }
        }
    }
}

/**
  * @brief  获取任务状态信息
  */
void TaskScheduler_GetStatus(TaskStatus* status, uint8_t* count)
{
    if (status != NULL && count != NULL) {
        *count = task_count;
        for (uint8_t i = 0; i < task_count; i++) {
            status[i].interval = task_list[i].interval;
            status[i].priority = task_list[i].priority;
            status[i].enabled = task_list[i].enabled;
            status[i].name = task_list[i].name;
        }
    }
}

/**
  * @brief  启动任务调度器
  */
void TaskScheduler_Start(void)
{
    scheduler_running = 1;
}

/**
  * @brief  停止任务调度器
  */
void TaskScheduler_Stop(void)
{
    scheduler_running = 0;
}

/**
  * @brief  获取任务数量
  */
uint8_t TaskScheduler_GetTaskCount(void)
{
    return task_count;
}
