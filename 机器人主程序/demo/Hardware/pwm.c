/**
  ******************************************************************************
  * @file    pwm.c
  * @author  生活中的小黑
  * @version V 2.0.0
  * @date    2025年8月4日
  * @brief   动作舵机控制模块
  ******************************************************************************
  */
  
#include "stm32f10x.h"
#include "delay.h"
#include <stdlib.h>
#include "pwm.h"
#include <stdio.h>
#include "face.h"
#include "touch.h"

// 全局变量
static uint8_t g_servoNow[7] = {90,90,90,90,90,90,90};  // 1~6号舵机当前角度
static ServoMotion g_servoMotions[6] = {0};  // 6个舵机的运动状态
static ServoCommand g_multiCommands[6] = {0}; // 多舵机命令
static uint8_t g_multiCommandCount = 0;
static uint32_t g_multiStartTime = 0;

// 序列命令全局变量
static ServoSequenceCommand g_sequence[64]; // 序列命令数组
static uint8_t g_sequence_count = 0;        // 序列命令总数
static uint8_t g_sequence_index = 0;        // 当前执行的命令索引
static uint32_t g_sequence_start_time = 0;  // 序列开始时间
static uint8_t g_sequence_running = 0;      // 序列运行标志
static uint8_t g_sequence_completed = 1; // 序列完成标志

// PWM初始化（保持不变）
void App_PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
    TIM_OCInitTypeDef TIM_OCInitStruct;
    
    // 1. 初始化GPIO引脚
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    // 配置所有舵机引脚
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;  // TIM1_CH1 (PA8)
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;  // TIM1_CH2 (PA9)
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;  // TIM2_CH1 (PA0)
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;  // TIM2_CH2 (PA1)
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;  // TIM2_CH3 (PA2)
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;  // TIM2_CH4 (PA3)
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 2. 配置TIM1时基单元
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_TimeBaseInitStruct.TIM_Period = 1999;
    TIM_TimeBaseInitStruct.TIM_Prescaler = 719;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = 0;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStruct);
    
    // 3. 配置TIM2时基单元
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseInitStruct.TIM_Period = 1999;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);
    
    // 4. 配置TIM1输出比较通道
    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStruct.TIM_Pulse = 150;
    
    TIM_OC1Init(TIM1, &TIM_OCInitStruct);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    
    TIM_OC2Init(TIM1, &TIM_OCInitStruct);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    
    // 5. 配置TIM2输出比较通道
    TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
    
    TIM_OC1Init(TIM2, &TIM_OCInitStruct);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    
    TIM_OC2Init(TIM2, &TIM_OCInitStruct);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    
    TIM_OC3Init(TIM2, &TIM_OCInitStruct);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    
    TIM_OC4Init(TIM2, &TIM_OCInitStruct);
    TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);
    
    // 6. 使能预加载和定时器
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_CCPreloadControl(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
    
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_CCPreloadControl(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    
    // 初始化舵机状态
    for(int i = 0; i < 6; i++) {
        g_servoMotions[i].id = i + 1;
        g_servoMotions[i].state = SERVO_IDLE;
        g_servoMotions[i].currentAngle = 90;
        g_servoMotions[i].targetAngle = 90;
    }
    
    // 初始化序列命令
    g_sequence_count = 0;
    g_sequence_index = 0;
    g_sequence_running = 0;
}

// 序列命令执行函数
void Servo_ExecuteSequence(uint8_t count, ServoSequenceCommand sequence[])
{
    if (count == 0 || count > 64) return;
    
    // 如果当前有序列正在运行，先停止
    if (g_sequence_running) {
        Servo_StopSequence();
    }
    
    g_sequence_count = count;
    g_sequence_index = 0;
    for (uint8_t i = 0; i < count; i++) {
        g_sequence[i] = sequence[i];
    }
    
    g_sequence_start_time = GetTick();
    g_sequence_running = 1;
    g_sequence_completed = 0;
    
    // 开始执行第一个命令
    ServoSequenceCommand* cmd = &g_sequence[0];
    if (cmd->type == SERVO_CMD_MOVE) {
        Servo_Set_NonBlocking(cmd->id, cmd->targetAngle, cmd->moveTime);
    }
    // 如果是延迟命令，不需要特别处理，在UpdateSequence中处理
}

// 序列命令更新函数
uint8_t Servo_UpdateSequence(void)
{
    if (!g_sequence_running || g_sequence_count == 0) {
        g_sequence_completed = 1;
        return 1;
    }
    
    ServoSequenceCommand* cmd = &g_sequence[g_sequence_index];
    uint32_t current_time = GetTick();
    
    if (cmd->type == SERVO_CMD_MOVE) {
        // 舵机运动命令
        if (Servo_GetState(cmd->id) == SERVO_COMPLETED) {
            // 当前命令完成，移动到下一个命令
            g_sequence_index++;
            if (g_sequence_index >= g_sequence_count) {
                // 所有命令完成
                g_sequence_running = 0;
                g_sequence_count = 0;
                g_sequence_completed = 1;
                return 1;
            }
            
            // 执行下一个命令
            cmd = &g_sequence[g_sequence_index];
            g_sequence_start_time = current_time;
            
            if (cmd->type == SERVO_CMD_MOVE) {
                Servo_Set_NonBlocking(cmd->id, cmd->targetAngle, cmd->moveTime);
            }
            // 如果是延迟命令，不需要特别处理
        }
    } else {
        // 延迟命令
        if (current_time - g_sequence_start_time >= cmd->moveTime) {
            // 延迟时间到，移动到下一个命令
            g_sequence_index++;
            if (g_sequence_index >= g_sequence_count) {
                // 所有命令完成
                g_sequence_running = 0;
                g_sequence_count = 0;
                g_sequence_completed = 1;
                return 1;
            }
            
            // 执行下一个命令
            cmd = &g_sequence[g_sequence_index];
            g_sequence_start_time = current_time;
            
            if (cmd->type == SERVO_CMD_MOVE) {
                Servo_Set_NonBlocking(cmd->id, cmd->targetAngle, cmd->moveTime);
            }
            // 如果是延迟命令，继续等待
        }
    }
    
    return 0;
}


// 检查序列是否正在运行
uint8_t Servo_IsSequenceRunning(void)
{
    return g_sequence_running;
}

// 停止序列执行
void Servo_StopSequence(void)
{
    // 停止序列，但将所有舵机转回中间位置
    for (int i = 1; i <= 6; i++) {
        Servo_Set_NonBlocking(i, 90, 300); // 快速回到中间位置
    }
    
    g_sequence_running = 0;
    g_sequence_count = 0;
    g_sequence_index = 0;
    g_sequence_completed = 1;
}

// 添加检查序列是否完成的函数
uint8_t Servo_IsSequenceCompleted(void)
{
    return g_sequence_completed;
}

// 阻塞式舵机控制（兼容旧代码）
void Servo_Set_Blocking(uint8_t id, uint8_t targetAngle, uint16_t moveTime)
{
    if(id == 0 || id > 6) return;
    if(targetAngle > 180) targetAngle = 180;
    
    uint8_t start = g_servoNow[id];
    int16_t diff = (int16_t)targetAngle - start;
    uint16_t dist = abs(diff);
    if(dist == 0) return;
    
    uint16_t steps = dist;
    int8_t dir = (diff > 0) ? 1 : -1;
    uint16_t delay = moveTime / steps;
    if(delay == 0) delay = 1;
    
    for(uint16_t i = 0; i <= steps; ++i) {
        uint8_t cur = start + dir * i;
        Servo_Set_simple(id, cur);
        g_servoNow[id] = cur;
        Delay_ms(delay);
    }
}

// 非阻塞舵机控制 - 设置运动命令
void Servo_Set_NonBlocking(uint8_t id, uint8_t targetAngle, uint16_t moveTime)
{
    if(id == 0 || id > 6) return;
    if(targetAngle > 180) targetAngle = 180;
    
    g_servoMotions[id-1].currentAngle = g_servoNow[id];
    g_servoMotions[id-1].targetAngle = targetAngle;
    g_servoMotions[id-1].moveTime = moveTime;
    g_servoMotions[id-1].startTime = GetTick();
    g_servoMotions[id-1].state = SERVO_MOVING;
}

// 非阻塞舵机控制 - 更新函数（需要在主循环中调用）
uint8_t Servo_Update_NonBlocking(void)
{
    uint8_t allCompleted = 1;
    uint32_t currentTime = GetTick();
    
    for(int i = 0; i < 6; i++) {
        if(g_servoMotions[i].state == SERVO_MOVING) {
            allCompleted = 0;
            
            uint32_t elapsed = currentTime - g_servoMotions[i].startTime;
            if(elapsed >= g_servoMotions[i].moveTime) {
                // 运动完成
                g_servoMotions[i].currentAngle = g_servoMotions[i].targetAngle;
                g_servoMotions[i].state = SERVO_COMPLETED;
                Servo_Set_simple(g_servoMotions[i].id, g_servoMotions[i].targetAngle);
                g_servoNow[g_servoMotions[i].id] = g_servoMotions[i].targetAngle;
            } else {
                // 计算当前角度
                float progress = (float)elapsed / g_servoMotions[i].moveTime;
                int16_t diff = (int16_t)g_servoMotions[i].targetAngle - g_servoMotions[i].currentAngle;
                uint8_t currentAngle = g_servoMotions[i].currentAngle + (diff * progress);
                
                Servo_Set_simple(g_servoMotions[i].id, currentAngle);
                g_servoNow[g_servoMotions[i].id] = currentAngle;
            }
        }
    }
    
    return allCompleted;
}

// 获取舵机状态
ServoState Servo_GetState(uint8_t id)
{
    if(id == 0 || id > 6) return SERVO_IDLE;
    return g_servoMotions[id-1].state;
}

// 等待所有舵机运动完成
void Servo_WaitAllComplete(void)
{
    while(!Servo_Update_NonBlocking()) {
        Delay_ms(10);
    }
}

// 多舵机非阻塞控制
void Multi_Servo_Move_NonBlocking(uint8_t num, ServoCommand commands[])
{
    if(num == 0 || num > 6) return;
    
    g_multiCommandCount = num;
    for(uint8_t i = 0; i < num; i++) {
        g_multiCommands[i] = commands[i];
    }
    
    g_multiStartTime = GetTick();
    
    // 设置所有舵机开始运动
    for(uint8_t i = 0; i < num; i++) {
        Servo_Set_NonBlocking(g_multiCommands[i].id, 
                             g_multiCommands[i].targetAngle, 
                             g_multiCommands[i].moveTime);
    }
}

// 更新多舵机运动状态
uint8_t Multi_Servo_Update(void)
{
    return Servo_Update_NonBlocking();
}

// 简单舵机设置（立即到达）
void Servo_Set_simple(uint8_t id, uint8_t angle)
{
    if(angle > 180) angle = 180;

    uint16_t pulse = 50 + (uint16_t)(angle * 200 / 180);
    
    switch(id) {
        case 1: TIM_SetCompare1(TIM1, pulse); break;
        case 2: TIM_SetCompare2(TIM1, pulse); break;
        case 3: TIM_SetCompare1(TIM2, pulse); break;
        case 4: TIM_SetCompare2(TIM2, pulse); break;
        case 5: TIM_SetCompare3(TIM2, pulse); break;
        case 6: TIM_SetCompare4(TIM2, pulse); break;
    }
    
    g_servoNow[id] = angle;
}

// 动作初始化
void Action_Init(void)
{
    Servo_Set_simple(1,90);  // TIM1_CH1 (PA8)点头
    Servo_Set_simple(2,90);  // TIM1_CH2 (PA9)摇头
    Servo_Set_simple(3,90);  // TIM2_CH1 (PA0)左手开合
    Servo_Set_simple(4,90);  // TIM2_CH2 (PA1)左手举放
    Servo_Set_simple(5,90);  // TIM2_CH3 (PA2)右手开合
    Servo_Set_simple(6,90);  // TIM2_CH4 (PA3)右手举放
    
    Delay_ms(100);
}

/**
  * @brief  获取舵机当前角度
  * @param  id: 舵机ID (1-6)
  * @retval 当前角度 (0-180)
  */
uint8_t Servo_GetCurrentAngle(uint8_t id)
{
    if (id == 0 || id > 6) return 90;
    return g_servoNow[id];
}

// ==================== 使用序列命令的非阻塞动作函数 ====================

void Head_Left_NonBlocking(void)
{
    // 向左看动作序列：转头 -> 延迟 -> 眨眼 -> 转回
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 2, 60, 100},   // 头部向左转
		{SERVO_CMD_MOVE, 3, 110, 100}, 
		{SERVO_CMD_DELAY, 0, 0, 100},
        {SERVO_CMD_MOVE, 3, 90, 100},  // 同时转头
        {SERVO_CMD_DELAY, 0, 0, 3000},  // 保持1秒
        {SERVO_CMD_MOVE, 2, 90, 100},   // 头部转回中间
        {SERVO_CMD_MOVE, 3, 110, 100},  
		{SERVO_CMD_DELAY, 0, 0, 100},
		{SERVO_CMD_MOVE, 3, 90, 100},// 同时转头
    };
    Servo_ExecuteSequence(9, sequence);
}

void Head_Right_NonBlocking(void)
{
    // 向右看动作序列：转头 -> 延迟 -> 眨眼 -> 转回
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 2, 120, 100},  // 头部向右转
        {SERVO_CMD_MOVE, 3, 110, 100},  
		{SERVO_CMD_DELAY, 0, 0, 100},
		{SERVO_CMD_MOVE, 3, 90, 100},// 同时转头
        {SERVO_CMD_DELAY, 0, 0, 3000},  // 保持1秒
        {SERVO_CMD_MOVE, 2, 90, 100},   // 头部转回中间
        {SERVO_CMD_MOVE, 3, 110, 100},  
		{SERVO_CMD_DELAY, 0, 0, 100},
		{SERVO_CMD_MOVE, 3, 90, 100},// 同时转头
    };
    Servo_ExecuteSequence(9, sequence);
}

void Head_Up_NonBlocking(void)
{
    // 向上看动作序列：抬头 -> 延迟 -> 回到中间
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 60, 400},   // 抬头
        {SERVO_CMD_DELAY, 0, 0, 1500},  // 保持1.5秒
        {SERVO_CMD_MOVE, 3, 90, 400}    // 回到中间
    };
    Servo_ExecuteSequence(3, sequence);
}

void Head_Down_NonBlocking(void)
{
    // 向下看动作序列：低头 -> 延迟 -> 回到中间
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 2, 90, 200},  // 低头
        {SERVO_CMD_MOVE, 3, 110, 200},    // 回到中间
        {SERVO_CMD_DELAY, 0, 0, 1500},  // 保持1.5秒
        {SERVO_CMD_MOVE, 3, 90, 200}    // 回到中间
    };
    Servo_ExecuteSequence(4, sequence);
}

void Head_LU_NonBlocking(void)
{
    // 向左上看动作序列
    ServoSequenceCommand sequence[] = {
		{SERVO_CMD_MOVE, 3, 110, 100}, // 抬头
		{SERVO_CMD_MOVE, 2, 60, 100},  // 点头
		{SERVO_CMD_DELAY, 0, 0, 100}, // 延迟50毫秒头回中
		{SERVO_CMD_MOVE, 3, 60, 100}, // 摇
		{SERVO_CMD_DELAY, 0, 0, 7000},
		{SERVO_CMD_MOVE, 3, 90, 100},
		{SERVO_CMD_MOVE, 2, 90, 100},
    };
    Servo_ExecuteSequence(7, sequence);
}

void Head_RU_NonBlocking(void)
{
    // 向右上看动作序列
    ServoSequenceCommand sequence[] = {
		{SERVO_CMD_MOVE, 2, 120, 100}, // 摇头回中
        {SERVO_CMD_MOVE, 3, 110, 100}, // 点头
        {SERVO_CMD_DELAY, 0, 0, 100}, // 延迟50毫秒
        {SERVO_CMD_MOVE, 3, 60, 100}, // 抬头
		{SERVO_CMD_DELAY, 0, 0, 7000},
		{SERVO_CMD_MOVE, 3, 90, 100},
		{SERVO_CMD_MOVE, 2, 90, 100},
    };
    Servo_ExecuteSequence(7, sequence);
}

void Head_LD_NonBlocking(void)
{
    // 向左下看动作序列
    ServoSequenceCommand sequence[] = {
		{SERVO_CMD_MOVE, 2, 60, 100},// 摇头回中
        {SERVO_CMD_MOVE, 3, 110, 100}, // 点头
		{SERVO_CMD_DELAY, 0, 0, 7000},
		{SERVO_CMD_MOVE, 3, 90, 100},
		{SERVO_CMD_MOVE, 2, 90, 100},
    };
    Servo_ExecuteSequence(5, sequence);
}

void Head_RD_NonBlocking(void)
{
    // 向右下看动作序列
    ServoSequenceCommand sequence[] = {
		{SERVO_CMD_MOVE, 2, 120, 100},// 摇头回中
        {SERVO_CMD_MOVE, 3, 110, 100}, // 点头
		{SERVO_CMD_DELAY, 0, 0, 7000},
		{SERVO_CMD_MOVE, 3, 90, 100},
		{SERVO_CMD_MOVE, 2, 90, 100},
    };
    Servo_ExecuteSequence(5, sequence);
}

void Head_Shake_NonBlocking(void)
{
    // 摇头动作序列：左右摇头多次
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 2, 60, 100},   // 向左转
        {SERVO_CMD_DELAY, 0, 0, 100},   // 短暂延迟
        {SERVO_CMD_MOVE, 2, 120, 100},  // 向右转
        {SERVO_CMD_DELAY, 0, 0, 100},   // 短暂延迟
        {SERVO_CMD_MOVE, 2, 90, 100},  // 向右转
        {SERVO_CMD_DELAY, 0, 0, 100},   // 短暂延迟
    };
    Servo_ExecuteSequence(6, sequence);
}

void Head_Nod_NonBlocking(void)
{
    // 点头动作序列：上下点头多次
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 110, 200},  // 低头
        {SERVO_CMD_DELAY, 0, 0, 100},   // 短暂延迟
        {SERVO_CMD_MOVE, 3, 60, 200},   // 抬头
        {SERVO_CMD_DELAY, 0, 0, 100},   // 短暂延迟
        {SERVO_CMD_MOVE, 3, 110, 200},  // 低头
        {SERVO_CMD_DELAY, 0, 0, 100},   // 短暂延迟
        {SERVO_CMD_MOVE, 3, 90, 200}    // 回到中间
    };
    Servo_ExecuteSequence(7, sequence);
}

void Head_Around_NonBlocking(void)
{
    // 环顾四周动作序列
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 60, 500},   // 向左
        {SERVO_CMD_DELAY, 0, 0, 500},   // 保持0.5秒
        {SERVO_CMD_MOVE, 3, 110, 500},  // 向右
        {SERVO_CMD_DELAY, 0, 0, 500},   // 保持0.5秒
        {SERVO_CMD_MOVE, 2, 60, 500},   // 向上
        {SERVO_CMD_DELAY, 0, 0, 500},   // 保持0.5秒
        {SERVO_CMD_MOVE, 2, 120, 500},  // 向下
        {SERVO_CMD_DELAY, 0, 0, 500},   // 保持0.5秒
        {SERVO_CMD_MOVE, 3, 90, 500},   // 回到中间
        {SERVO_CMD_MOVE, 2, 90, 500}    // 回到中间
    };
    Servo_ExecuteSequence(10, sequence);
}

void Head_Response_NonBlocking(void)
{
    // 响应动作序列：快速点头
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 110, 100},  // 快速低头
        {SERVO_CMD_DELAY, 0, 0, 200},   // 保持0.2秒
        {SERVO_CMD_MOVE, 3, 90, 100}    // 快速抬头
    };
    Servo_ExecuteSequence(3, sequence);
}

void Action_Response_RiseHand_NonBlocking(void)
{
    // 响应动作序列
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 110, 200},  // 低头
        {SERVO_CMD_DELAY, 0, 0, 1000},  // 保持1秒
        {SERVO_CMD_MOVE, 3, 90, 200},   // 抬头
    };
    Servo_ExecuteSequence(5, sequence);
}

void Action_Shy_NonBlocking(void)
{
    // 害羞动作序列
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 60, 500},  // 头部保持
        {SERVO_CMD_MOVE, 2, 60, 500},  // 轻微转头
        {SERVO_CMD_DELAY, 0, 0, 500},  // 保持1秒
        {SERVO_CMD_MOVE, 2, 120, 500},   // 头转回
        {SERVO_CMD_DELAY, 0, 0, 500},  // 保持1秒
        {SERVO_CMD_MOVE, 2, 60, 500},   // 头转回
        {SERVO_CMD_DELAY, 0, 0, 500},  // 保持1秒
        {SERVO_CMD_MOVE, 2, 120, 500},   // 头转回
        {SERVO_CMD_MOVE, 2, 90, 500},   // 头转回
    };
    Servo_ExecuteSequence(8, sequence);
}

void Action_Smile_NonBlocking(void)
{
    // 微笑动作序列：轻微点头
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 110, 500},  // 低头微笑
        {SERVO_CMD_DELAY, 0, 0, 1000},  // 保持1秒
        {SERVO_CMD_MOVE, 3, 90, 500}    // 回到中间
    };
    Servo_ExecuteSequence(3, sequence);
}

void Action_Sad_NonBlocking(void)
{
    // 悲伤动作序列
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 110, 2000},  // 头部保持
        {SERVO_CMD_MOVE, 2, 90, 2000},  // 头部保持
		{SERVO_CMD_DELAY, 0, 0, 6000},  // 保持1秒
		{SERVO_CMD_MOVE, 3, 90, 500},  // 头部保持
    };
    Servo_ExecuteSequence(4, sequence);
}

void Action_Comfortable_NonBlocking(void)
{
    // 舒适动作序列
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 90, 2000},  // 头部保持
        {SERVO_CMD_MOVE, 2, 60, 2000},  // 轻微转头
        {SERVO_CMD_DELAY, 0, 0, 3000},  // 保持舒适姿势3秒
        {SERVO_CMD_MOVE, 2, 90, 1000}   // 转回中间
    };
    Servo_ExecuteSequence(4, sequence);
}

// ==================== 新增复杂动作函数 ====================

void Complex_Eat_NonBlocking(void)
{
    // 复杂进食动作序列：多次咀嚼 + 吞咽
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 110, 300},  // 低头准备
        {SERVO_CMD_DELAY, 0, 0, 3000},   // 准备延迟
        {SERVO_CMD_MOVE, 3, 90, 500},   // 抬头
    };
    Servo_ExecuteSequence(3, sequence);
}

/**
  * @brief  复杂舞蹈动作：低头并左转头后抬头，再低头并右转头后抬头，反复6遍（非阻塞序列版）
  */
void Complex_Dance_NonBlocking(void)
{
    #define DANCE_LOOP_COUNT 6
    #define MOVE_TIME 200      // 舵机运动时间(ms)
    #define HOLD_TIME 100      // 姿态保持时间(ms)

    // 最大命令数：每个完整循环(左+右)需 10 条命令，6 循环 60 条，加最后回中 2 条，共 62 条
    ServoSequenceCommand sequence[64];
    uint8_t idx = 0;

    for (uint8_t i = 0; i < DANCE_LOOP_COUNT; i++) {
        // ----- 左半循环：低头 + 左转 -----
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 3, 110, MOVE_TIME};  // 低头
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 2, 60,  MOVE_TIME};  // 左转
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_DELAY, 0, 0, MOVE_TIME};   // 等待动作完成
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 3, 90,  MOVE_TIME};  // 抬头回中
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_DELAY, 0, 0, HOLD_TIME};   // 短暂保持

        // ----- 右半循环：低头 + 右转 -----
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 3, 110, MOVE_TIME};  // 低头
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 2, 120, MOVE_TIME};  // 右转
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_DELAY, 0, 0, MOVE_TIME};   // 等待动作完成
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 3, 90,  MOVE_TIME};  // 抬头回中
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_DELAY, 0, 0, HOLD_TIME};   // 短暂保持
    }

    // 舞蹈结束，头部摆正（舵机2回中，舵机3已回中）
    sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 2, 90, MOVE_TIME};
    sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_DELAY, 0, 0, HOLD_TIME};

    Servo_ExecuteSequence(idx, sequence);
}

/**
  * @brief  挣扎动作：左右摆头反复执行3遍（左→右→左→右→左→右）
  */
void Action_Struggle_NonBlocking(void)
{
    ServoSequenceCommand sequence[20];
    uint8_t idx = 0;

    // 左右摆头3遍 = 6次半次摆动 (左、右、左、右、左、右)
    for (uint8_t i = 0; i < 3; i++) {
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 2, 60, 100};   // 左转
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_DELAY, 0, 0, 100};
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 2, 120, 100};  // 右转
        sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_DELAY, 0, 0, 100};
    }
    // 最后回到中间
    sequence[idx++] = (ServoSequenceCommand){SERVO_CMD_MOVE, 2, 90, 100};

    Servo_ExecuteSequence(idx, sequence);
}
