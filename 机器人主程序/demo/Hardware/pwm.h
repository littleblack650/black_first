/**
  ******************************************************************************
  * @file    pwm.c
  * @author  生活中的小黑
  * @version V 2.0.0
  * @date    2025年8月4日
  * @brief   动作舵机控制头文件
  ******************************************************************************
  */
  
#ifndef _PWM_H_
#define _PWM_H_

#include "stm32f10x.h"

// 舵机状态枚举
typedef enum {
    SERVO_IDLE = 0,     // 空闲状态
    SERVO_MOVING,       // 运动中
    SERVO_COMPLETED     // 运动完成
} ServoState;

// 命令类型枚举
typedef enum {
    SERVO_CMD_MOVE = 0,  // 舵机运动命令
    SERVO_CMD_DELAY      // 延迟命令
} ServoCommandType;

// 舵机控制命令结构
typedef struct {
    uint8_t id;
    uint8_t targetAngle;
    uint16_t moveTime;  // 运动时间(ms)
} ServoCommand;

// 舵机状态结构
typedef struct {
    uint8_t id;
    uint8_t currentAngle;
    uint8_t targetAngle;
    uint16_t moveTime;
    uint32_t startTime;
    ServoState state;
} ServoMotion;

// 序列命令结构
typedef struct {
    uint8_t type;        // 0:舵机命令, 1:延迟命令
    uint8_t id;          // 舵机ID (type=0时有效)
    uint8_t targetAngle; // 目标角度 (type=0时有效)
    uint16_t moveTime;   // 运动时间或延迟时间
} ServoSequenceCommand;

// 函数声明
void App_PWM_Init(void);
void Servo_Set_Blocking(uint8_t id, uint8_t targetAngle, uint16_t moveTime);
void Servo_Set_NonBlocking(uint8_t id, uint8_t targetAngle, uint16_t moveTime);
uint8_t Servo_Update_NonBlocking(void);
ServoState Servo_GetState(uint8_t id);
void Servo_WaitAllComplete(void);

// 多舵机控制
void Multi_Servo_Move_NonBlocking(uint8_t num, ServoCommand commands[]);
uint8_t Multi_Servo_Update(void);

// 序列命令控制
void Servo_ExecuteSequence(uint8_t count, ServoSequenceCommand sequence[]);
uint8_t Servo_UpdateSequence(void);
uint8_t Servo_IsSequenceRunning(void);
void Servo_StopSequence(void);
uint8_t Servo_IsSequenceCompleted(void);// 添加序列完成检查函数

// 表情配套动作函数
void Action_Init(void);
void Head_Left_NonBlocking(void);
void Head_Right_NonBlocking(void);
void Head_Up_NonBlocking(void);
void Head_Down_NonBlocking(void);
void Head_LU_NonBlocking(void);
void Head_RU_NonBlocking(void);
void Head_LD_NonBlocking(void);
void Head_RD_NonBlocking(void);
void Head_Shake_NonBlocking(void);
void Head_Nod_NonBlocking(void);
void Head_Around_NonBlocking(void);
void Head_Response_NonBlocking(void);
void Action_Response_RiseHand_NonBlocking(void);
void Action_Shy_NonBlocking(void);
void Action_Struggle_NonBlocking(void);
void Action_Smile_NonBlocking(void);
void Action_Sad_NonBlocking(void);
void Action_Comfortable_NonBlocking(void);
void Complex_Eat_NonBlocking(void);
void Complex_Dance_NonBlocking(void);
uint8_t Servo_GetCurrentAngle(uint8_t id);//获得当前角度用于休眠模式

// 简单阻塞函数（兼容旧代码）
void Servo_Set_simple(uint8_t id, uint8_t angle);

#endif
