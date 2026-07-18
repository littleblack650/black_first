/**
  ******************************************************************************
  * @file    pwm.c
  * @author  生活中的小黑
  * @version V 1.0.0
  * @date    2025年7月23日
  * @brief   桌宠舵机动作函数源文件
  ******************************************************************************
  */
#include "stm32f10x.h"
#include "delay.h"
#include <stdlib.h>   // abs()
#include "pwm.h"
#include <stdio.h>    // 用于调试输出

void App_PWM_Init(void);
void Servo_Set(uint8_t id, uint8_t targetAngle, uint16_t totalTime_ms, uint8_t fromAngle);//设置各舵机角度，梯形加减速，启停更平滑（Servo_Set(舵机号, 目标角度, 总时间_ms, 起始角度);）
void Servo_Set_simple(uint8_t id, uint8_t angle);//设置各舵机角度，简单无控速
void Action_Init(void);//动作初始化
void Head_Lest(void);//向左
void Head_Right(void);//向右
void Head_Up(void);//向上
void Head_Down(void);//向下
void Head_LU(void);//向左上角
void Head_RU(void);//向右上角
void Head_LD(void);//向左下角
void Head_RD(void);//向右下角
void Head_Shake(void);//摇头
void Head_nod(void);//点头
void Head_around(void);//环视
void Head_Response(void);//答应（普通）
void Action_Response_RiseHand(void);//答应（举右手）
void Action_Shy(void);//害羞（先低头再摇头，双手举起并收拢）
void Action_Struggle(void);//挣扎（摇头，张开收拢往复）
void Action_Smile(void);//笑（点头往复）
void Action_Sad(void);//伤心（低头，抬手90度收拢张开往复）
void Action_Comfortable(void);//舒服（抬头并摇头）

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
    TIM_OCInitStruct.TIM_Pulse = 150; // 初始90度位置
    
    TIM_OC1Init(TIM1, &TIM_OCInitStruct);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);  // 关键添加
    
    TIM_OC2Init(TIM1, &TIM_OCInitStruct);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);  // 关键添加
    
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
}

/* 保存 6 路当前角度，方便"连续调用"时作为起始值 */
static uint8_t g_servoNow[7] = {90,90,90,90,90,90,90};  /* 1~6 号舵机 */

void Servo_Set(uint8_t id,
               uint8_t targetAngle,
               uint16_t totalTime_ms,
               uint8_t fromAngle) // Servo_Set(舵机号, 目标角度, 总时间_ms, 起始角度);
{
    /* 参数检查 */
    if(id == 0 || id > 6) return;
    if(targetAngle > 180) targetAngle = 180;

    /* 取当前角度作为起点 */
    uint8_t start = (fromAngle == 0xFF) ? g_servoNow[id] : fromAngle;
    int16_t diff  = (int16_t)targetAngle - start;
    uint16_t dist = abs(diff);
    if(dist == 0) { g_servoNow[id] = targetAngle; return; }

    /* 匀速运动参数 */
    uint16_t steps = dist;   // 总步数 = 角度差
    int8_t   dir   = (diff > 0) ? 1 : -1;
    uint16_t delay = totalTime_ms / steps; // 每步延时

    /* 确保最小延时 */
    if(delay == 0) delay = 1;

    /* 主循环：每步固定延时 */
    for(uint16_t i = 0; i <= steps; ++i)
    {
        uint8_t cur = start + dir * i;
        
        /* PWM脉冲宽度计算 */
        uint16_t pulse = 50 + (cur * 10) / 9;
        
        /* 调用底层 PWM 设置 */
        switch(id)
        {
            case 1: TIM_SetCompare1(TIM1, pulse); break;
            case 2: TIM_SetCompare2(TIM1, pulse); break;
            case 3: TIM_SetCompare1(TIM2, pulse); break;
            case 4: TIM_SetCompare2(TIM2, pulse); break;
            case 5: TIM_SetCompare3(TIM2, pulse); break;
            case 6: TIM_SetCompare4(TIM2, pulse); break;
        }
        g_servoNow[id] = cur;

        Delay_ms(delay);
    }
}
void Servo_Set_simple(uint8_t id, uint8_t angle)
{
    if(angle > 180) angle = 180;

    uint16_t pulse = 50 + (uint16_t)(angle * 200 / 180);
    
    switch(id)
    {
        case 1: TIM_SetCompare1(TIM1, pulse); break;
        case 2: TIM_SetCompare2(TIM1, pulse); break;
        case 3: TIM_SetCompare1(TIM2, pulse); break;
        case 4: TIM_SetCompare2(TIM2, pulse); break;
        case 5: TIM_SetCompare3(TIM2, pulse); break;
        case 6: TIM_SetCompare4(TIM2, pulse); break;
    }
}

void Action_Init(void)
{
    // 添加调试信息
    
    Servo_Set_simple(1,90);  // TIM1_CH1 (PA8)点头
    Servo_Set_simple(2,90);  // TIM1_CH2 (PA9)摇头
    Servo_Set_simple(3,90);  // TIM2_CH1 (PA0)左手开合
    Servo_Set_simple(4,90);  // TIM2_CH2 (PA1)左手举放
    Servo_Set_simple(5,90);  // TIM2_CH3 (PA2)右手开合
    Servo_Set_simple(6,90);  // TIM2_CH4 (PA3)右手举放
    
    Delay_ms(100);  // 确保舵机到位
}

void Head_Lest(void)//向左
{
	Servo_Set(2, 45, 20, 90);
}
void Head_Right(void)//向右
{}
void Head_Up(void)//向上
{}
void Head_Down(void)//向下
{}
void Head_LU(void)//向左上角
{}
void Head_RU(void)//向右上角
{}
void Head_LD(void)//向左下角
{}
void Head_RD(void)//向右下角
{}
void Head_Shake(void)//摇头
{}
void Head_nod(void)//点头
{}
void Head_around(void)//环视
{}
void Head_Response(void)//答应（普通）
{}
void Action_Response_RiseHand(void)//答应（举右手）
{}
void Action_Shy(void)//害羞（先低头再摇头，双手举起并收拢）
{}
void Action_Struggle(void)//挣扎（摇头，张开收拢往复）
{}
void Action_Smile(void)//笑（点头往复）
{}
void Action_Sad(void)//伤心（低头，抬手90度收拢张开往复）
{}
void Action_Comfortable(void)//舒服（抬头并摇头）
{}
//睡觉（躺下后触摸会慢摇头）最后开发
