/**
  ******************************************************************************
  * @file    usart.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   串口通信模块头文件，定义语音指令枚举
  ******************************************************************************
  */
#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdio.h>

/* 语音指令枚举 */
typedef enum {
    VOICE_CMD_NONE = 0x00,          // 无指令
    
    /* 0x01-0x22 语音指令 */
    VOICE_CMD_01 = 0x01,          //小鱼干
    VOICE_CMD_02 = 0x02,          //你好BT
    VOICE_CMD_03 = 0x03,          //揍你
    VOICE_CMD_04 = 0x04,          //笑一个
    VOICE_CMD_05 = 0x05,          //最小声
    VOICE_CMD_06 = 0x06,          //中等音量
    VOICE_CMD_07 = 0x07,          //最大声
    VOICE_CMD_08 = 0x08,          //大点声
    VOICE_CMD_09 = 0x09,          //小点声
    VOICE_CMD_0A = 0x0A,          //闭嘴
    VOICE_CMD_0B = 0x0B,          //说话
    VOICE_CMD_0C = 0x0C,          //唱首歌
    VOICE_CMD_0D = 0x0D,          //我爱你
    VOICE_CMD_0E = 0x0E,          //往上看
    VOICE_CMD_0F = 0x0F,          //狠一点
    VOICE_CMD_10 = 0x10,          //
    VOICE_CMD_11 = 0x11,          //再狠一点
    VOICE_CMD_12 = 0x12,          //干他
    VOICE_CMD_13 = 0x13,          //哭一个
    VOICE_CMD_14 = 0x14,          //伤心
    VOICE_CMD_15 = 0x15,          //
    VOICE_CMD_16 = 0x16,          //我帅吗
    VOICE_CMD_17 = 0x17,          //吃饭
    VOICE_CMD_18 = 0x18,          //呕吐
    VOICE_CMD_19 = 0x19,          //生气
    VOICE_CMD_1A = 0x1A,          //往下看
    VOICE_CMD_1B = 0x1B,          //往左看
    VOICE_CMD_1C = 0x1C,          //往右看
    VOICE_CMD_1D = 0x1D,          //给大家拜个年
    VOICE_CMD_1E = 0x1E,          //睡觉吧
    VOICE_CMD_1F = 0x1F,          //起床啦
    VOICE_CMD_20 = 0x20,          //老大
    VOICE_CMD_21 = 0x21,          //老大我想你了
    VOICE_CMD_22 = 0x22,		  //立正
    VOICE_CMD_23 = 0x23,          //放个烟花

    VOICE_CMD_MAX = 0x40            // 指令最大值
} VoiceCommand;

/* 语音指令回调函数类型 */
typedef void (*VoiceCommandCallback)(VoiceCommand cmd);

/* 函数声明 */

/**
  * @brief  USART1初始化函数
  * @note   使用PB6(TX)和PB7(RX)作为USART1引脚，波特率9600
  * @param  无
  * @retval 无
  */
void USART1_Config(void);

/**
  * @brief  注册语音指令回调函数
  * @param  callback: 回调函数指针
  * @retval 无
  */
void USART_RegisterVoiceCallback(VoiceCommandCallback callback);

/**
  * @brief  获取最后接收到的语音指令
  * @param  无
  * @retval 最后接收到的语音指令
  */
VoiceCommand USART_GetLastVoiceCommand(void);

/**
  * @brief  清除语音指令缓存
  * @param  无
  * @retval 无
  */
void USART_ClearVoiceCommand(void);

/**
  * @brief  检查是否有新的语音指令
  * @param  无
  * @retval 1:有新指令, 0:无指令
  */
uint8_t USART_HasNewVoiceCommand(void);

/**
  * @brief  处理接收缓冲区中的所有数据，应在主循环中调用
  * @param  无
  * @retval 1:处理了指令, 0:无指令处理
  */
uint8_t USART_ProcessVoiceData(void);

void USART1_IRQHandler_Impl(void);

#endif
