/**
  ******************************************************************************
  * @file    module_interface.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   模块接口标准 - 所有模块都应遵循此接口
  ******************************************************************************
  */

#ifndef _MODULE_INTERFACE_H_
#define _MODULE_INTERFACE_H_

#include "stm32f10x.h"

/* 模块状态枚举 */
typedef enum {
    MODULE_STATE_UNINIT = 0,    // 未初始化
    MODULE_STATE_READY,         // 就绪
    MODULE_STATE_RUNNING,       // 运行中
    MODULE_STATE_ERROR,         // 错误
    MODULE_STATE_SLEEP          // 睡眠
} ModuleState;

/* 模块接口结构体 */
typedef struct {
    // 基本操作
    void (*Init)(void);         // 初始化函数
    void (*Start)(void);        // 启动函数
    void (*Stop)(void);         // 停止函数
    void (*Update)(void);       // 更新函数
    
    // 状态管理
    ModuleState (*GetState)(void);              // 获取状态
    uint8_t (*IsReady)(void);                   // 是否就绪
    const char* (*GetName)(void);               // 获取模块名称
    
    // 错误处理
    uint8_t (*HasError)(void);                  // 是否有错误
    const char* (*GetErrorString)(void);        // 获取错误信息
} ModuleInterface;

/* 传感器模块接口扩展 */
typedef struct {
    ModuleInterface base;       // 基础接口
    
    // 传感器特有接口
    void (*Calibrate)(void);                    // 校准函数
    float (*GetData)(uint8_t data_type);        // 获取数据
    void (*SetSensitivity)(float sensitivity);  // 设置灵敏度
} SensorModuleInterface;

/* 执行器模块接口扩展 */
typedef struct {
    ModuleInterface base;       // 基础接口
    
    // 执行器特有接口
    void (*SetPosition)(uint8_t id, float position);    // 设置位置
    void (*SetSpeed)(uint8_t id, float speed);          // 设置速度
    uint8_t (*IsMoving)(uint8_t id);                    // 是否在运动
} ActuatorModuleInterface;

#endif
