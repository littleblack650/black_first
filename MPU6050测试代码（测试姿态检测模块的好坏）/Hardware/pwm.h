/**
  ******************************************************************************
  * @file    pwm.c
  * @author  生活中的小黑
  * @version V 1.0.0
  * @date    2025年7月23日
  * @brief   桌宠舵机动作函数源文件
  ******************************************************************************
  */

#ifndef _PWM_H_
#define _PWM_H_

#include "stm32f10x.h"

void App_PWM_Init(void);
void Servo_Set(uint8_t id, uint8_t targetAngle, uint16_t Time_ms, uint8_t fromAngle);//设置各舵机角度，梯形加减速，启停更平滑（Servo_Set(舵机号, 目标角度, 总时间_ms, 起始角度);）
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

#endif
