/**
  ******************************************************************************
  * @file    system_config.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   系统配置文件 - 统一管理所有配置参数
  ******************************************************************************
  */

#ifndef _SYSTEM_CONFIG_H_
#define _SYSTEM_CONFIG_H_

#include "stm32f10x.h"

/* 系统时钟配置 */
#define SYSTEM_CLOCK_FREQ    72000000  // 系统主频72MHz

/* 任务调度配置 */
#define TASK_SCHEDULER_FREQ  100       // 任务调度频率100Hz
#define TASK_PRIORITY_HIGH   0         // 高优先级任务
#define TASK_PRIORITY_MEDIUM 1         // 中优先级任务  
#define TASK_PRIORITY_LOW    2         // 低优先级任务

/* MPU6050配置 */
#define MPU6050_UPDATE_FREQ  200       // MPU6050更新频率200Hz
#define SHAKE_THRESHOLD      3.0f      // 晃动检测阈值
#define DIZZY_TRIGGER_INTERVAL 1000    // 头晕触发间隔(ms)
#define DIZZY_EXTEND_DURATION 3000     // 头晕延长持续时间(ms)
#define VOMIT_RECOVERY_TIME  3000      // 呕吐恢复时间(ms)

/* 触摸传感器配置 */
#define TOUCH_DEBOUNCE_MS          200     // 触摸消抖时间(ms)
#define TOUCH_STATE_CHANGE_THRESHOLD 2000  // 触摸状态变化阈值(ms)
#define TOUCH_DELAY_TIME           500     // 触摸后延迟时间(ms)
#define SHY_ACTION_INTERVAL        300     // 害羞动作间隔(ms)

/* 表情系统配置 */
#define FACE_UPDATE_FREQ     50       // 表情更新频率50Hz
#define RANDOM_FACE_ENABLE   1        // 启用随机表情

/* 舵机系统配置 */
#define SERVO_UPDATE_FREQ    100      // 舵机更新频率100Hz
#define SERVO_COUNT          6        // 舵机数量

/* 通信配置 */
#define UART_BAUDRATE        9600     // 串口波特率
#define VOICE_CMD_INTERVAL   300      // 语音指令处理间隔(ms)

/* 存储配置 */
#define FLASH_SECTOR_SIZE    4096     // Flash扇区大小
#define FLASH_PAGE_SIZE      256      // Flash页大小

/* 调试配置 */
#define DEBUG_ENABLE         1        // 启用调试输出
#define DEBUG_UART           USART1   // 调试串口

/* 电池检测配置 */
#define BATTERY_CHECK_INTERVAL   5000    // 电池检测间隔5秒
#define LOW_VOLTAGE_THRESHOLD    3.4f    // 低电压阈值(约1/3电量)
#define CHARGING_VOLTAGE         4.0f    // 充电检测电压阈值
#define VOLTAGE_NORMAL_THRESHOLD 3.6f    // 恢复正常电压阈值

#endif
