/**
  ******************************************************************************
  * @file    random.h
  * @author  生活中的小黑
  * @version V 1.0.0
  * @date    2025年8月4日
  * @brief   表情随机选择系统头文件
  ******************************************************************************
  */

#ifndef _RANDOM_H_
#define _RANDOM_H_

#include "stm32f10x.h"
#include "face.h"

/* 表情概率配置结构体 */
typedef struct {
    FaceType face_type;      // 表情类型
    uint8_t probability;     // 出现概率 (0-100)
    uint32_t min_duration;   // 最小持续时间(ms)
    uint32_t max_duration;   // 最大持续时间(ms)
} FaceProbability;

/* 随机表情系统配置 */
typedef struct {
    const FaceProbability* probability_table;    // 概率表指针
    uint8_t table_size;                          // 概率表大小
    uint8_t enable_blink_between;                // 是否在表情间插入眨眼
    uint32_t min_interval;                       // 表情最小间隔时间(ms)
    uint32_t max_interval;                       // 表情最大间隔时间(ms)
} RandomFaceConfig;

/* 随机表情系统状态 */
typedef struct {
    RandomFaceConfig config;         // 当前配置
    uint32_t last_face_time;         // 上次切换表情时间
    uint32_t current_face_end_time;  // 当前表情结束时间
    uint8_t is_playing_face;         // 是否正在播放表情
} RandomFaceState;

/* 函数声明 */
void RandomFace_Init(const RandomFaceConfig* config);
void RandomFace_Update(void);
void RandomFace_TriggerNow(void);
void RandomFace_SetEnable(uint8_t enable);
uint8_t RandomFace_IsEnabled(void);
const RandomFaceConfig* RandomFace_GetDefaultConfig(void);

#endif
