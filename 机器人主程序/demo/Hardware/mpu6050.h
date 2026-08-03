/**
  ******************************************************************************
  * @file    mpu6050.h
  * @author  生活中的小黑
  * @version V 2.0.0
  * @date    2025年8月4日
  * @brief   MPU6050传感器驱动与运动检测模块头文件
  ******************************************************************************
  */

#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "face.h"

/* MPU6050状态枚举定义 */
typedef enum {
    MPU_STATE_NORMAL = 0,      // 正常状态，无特殊动作
    MPU_STATE_SHAKING,         // 晃动状态，正在摇晃
    MPU_STATE_DIZZY_EXTEND,    // 头晕延长状态，停止晃动后持续3秒
    MPU_STATE_VOMIT_PLAYING,   // 呕吐表情播放中
    MPU_STATE_RECOVERING,       // 恢复状态
	MPU_STATE_LYING_DOWN       // 新增：躺倒状态
} MPUState;

/* 动作回调函数类型定义 */
typedef void (*MPUActionCallback)(FaceType face_type);

/* 新增：躺倒状态回调类型 */
typedef void (*MPULieCallback)(void);

/* 公开函数声明 */

/**
  * @brief  MPU6050初始化
  */
void App_MPU6050_Init(void);

/**
  * @brief  MPU6050数据处理（旧名，保留兼容）
  */
void App_MPU6050_Proc(void);

/**
  * @brief  更新MPU6050数据和状态（建议5ms调用一次）
  */
void App_MPU6050_Update(void);

void MPU6050_RegisterLieCallback(MPULieCallback callback);      // 注册躺倒回调
void MPU6050_RegisterStandCallback(MPULieCallback callback);    // 注册站立恢复回调

/**
  * @brief  注册动作回调函数
  * @param  callback: 回调函数指针
  */
void MPU6050_RegisterActionCallback(MPUActionCallback callback);

/**
  * @brief  设置晃动检测灵敏度
  * @param  shake_threshold: 晃动阈值，值越小越灵敏
  */
void MPU6050_SetShakeSensitivity(float shake_threshold);

/**
  * @brief  设置基于Y轴加速度的晃动判定阈值（单位 g）
  * @param thresh 加速度阈值（例如 0.5 表示 0.5g）
  */
void MPU6050_SetAccelShakeThreshold(float thresh);

/**
  * @brief  获取当前MPU状态
  * @retval 当前状态
  */
MPUState MPU6050_GetCurrentState(void);

/**
  * @brief  判断是否允许执行其他动作
  * @retval 1:允许, 0:不允许
  */
uint8_t MPU6050_IsAllowOtherActions(void);

/**
  * @brief  通知呕吐表情播放完成
  */
void MPU6050_NotifyVomitComplete(void);

/**
 * @brief 忽略一段时间内 MPU 发起的动作回调，避免舵机动作自触发
 * @param ms  忽略时间（毫秒）
 */
void MPU6050_IgnoreActionsFor(uint32_t ms);

/* 新增：强制退出躺倒状态（用于触摸唤醒后外部恢复） */
void MPU6050_ForceExitLieState(void);

/**
  * @brief  获取加速度计数据
  */
float App_MPU6050_GetAccelX(void);
float App_MPU6050_GetAccelY(void);
float App_MPU6050_GetAccelZ(void);
float App_MPU6050_GetGyroX(void);
float App_MPU6050_GetGyroY(void);
float App_MPU6050_GetGyroZ(void);
float App_MPU6050_GetTemperature(void);

/**
  * @brief  获取姿态角度
  */
float App_MPU6050_GetYaw(void);
float App_MPU6050_GetRoll(void);
float App_MPU6050_GetPitch(void);

#endif
