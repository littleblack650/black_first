/**
  ******************************************************************************
  * @file    random.c
  * @author  生活中的小黑
  * @version V 1.0.0
  * @date    2025年8月4日
  * @brief   表情随机选择系统源文件
  ******************************************************************************
  */

#include "random.h"
#include "Delay.h"
#include "face.h"
#include <stddef.h>  // 用于NULL定义

/* 随机表情状态 */
static RandomFaceState random_face_state = {0};
static uint8_t random_face_enabled = 0;

/* 默认表情概率表 */
static const FaceProbability default_probability_table[] = {
    // 表情类型           概率  最小持续时间(ms)  最大持续时间(ms)
    {FACE_BLINK,         25,   4000,        6000},    // 眨眼: 25%概率, 持续4-6秒
    {FACE_LEFT,          12,   4000,        8000},    // 向左: 12%概率, 持续4-8秒
    {FACE_SMILE,         15,   4000,        8000},    // 微笑: 15%概率, 持续4-8秒
    {FACE_RIGHT,         15,   3000,        4000},    // 向右: 15%概率, 持续3-4秒
    {FACE_UP,            15,   3000,        4000},    // 向上: 15%概率, 持续3-4秒
    {FACE_DOWN,          15,   3000,        35000},   // 向下: 15%概率, 持续3-35秒
    {FACE_LEFTUP,        12,   4000,        8000},
    {FACE_RIGHTUP,       12,   4000,        8000},
    {FACE_LEFTDOWN,      15,   4000,        8000},
    {FACE_RIGHTDOWN,     15,   4000,        8000},
    {FACE_DOUBT,         18,   2000,        4000},    // 疑惑: 18%概率, 持续2-4秒
    {FACE_SHY,           4,    3000,        5000},    // 害羞: 4%概率, 持续3-5秒
    {FACE_COMFORTABLE,   5,    4000,        7000},    // 舒适: 5%概率, 持续4-7秒
    {FACE_SAD,           15,   5000,        8000},    // 悲伤: 15%概率, 持续5-8秒
    {FACE_CRY,           15,   4000,        6000},    // 哭泣: 15%概率, 持续4-6秒
    {FACE_ANGRY,         15,   2000,        5000},    // 生气: 15%概率, 持续2-5秒
    {FACE_LOVE,          15,   6000,        8000},    // 爱心: 15%概率, 持续6-8秒
    {FACE_EAT,           10,   5000,        8000},    // 吃东西: 10%概率, 持续5-8秒
    {FACE_SLEEP,         4,    8000,        15000}    // 睡觉: 4%概率, 持续8-15秒
};

/* 默认随机表情配置 */
static const RandomFaceConfig default_config = {
    .probability_table = default_probability_table,
    .table_size = sizeof(default_probability_table) / sizeof(default_probability_table[0]),
    .enable_blink_between = 1,    // 表情间插入眨眼
    .min_interval = 2000,         // 最小间隔2秒
    .max_interval = 10000         // 最大间隔10秒
};

/* 静态函数声明 */
static FaceType SelectRandomFace(void);
static uint32_t GetRandomInRange(uint32_t min, uint32_t max);
static uint8_t ShouldPlayNewFace(void);
static void PlayRandomFace(void);

/**
  * @brief  初始化随机表情系统
  * @param  config: 随机表情配置，为NULL则使用默认配置
  */
void RandomFace_Init(const RandomFaceConfig* config)
{
    if (config != NULL) {
        random_face_state.config = *config;
    } else {
        random_face_state.config = default_config;
    }
    
    random_face_state.last_face_time = GetTick();
    random_face_state.current_face_end_time = 0;
    random_face_state.is_playing_face = 0;
    random_face_enabled = 1;
}

/**
  * @brief  更新随机表情系统（需在主循环中定期调用）
  */
void RandomFace_Update(void)
{
    if (!random_face_enabled) {
        return;
    }
    
    uint32_t current_time = GetTick();
    
    if (random_face_state.is_playing_face) {
        if (current_time >= random_face_state.current_face_end_time) {
            random_face_state.is_playing_face = 0;
            
            if (random_face_state.config.enable_blink_between) {
                Face_RequestExpression(FACE_BLINK, FACE_PRIORITY_LOW, 0);
            }
            
            random_face_state.last_face_time = current_time;
        }
        return;
    }
    
    if (ShouldPlayNewFace()) {
        PlayRandomFace();
    }
}

/**
  * @brief  立即触发一个随机表情
  */
void RandomFace_TriggerNow(void)
{
    if (!random_face_enabled) {
        return;
    }
    PlayRandomFace();
}

/**
  * @brief  启用/禁用随机表情系统
  */
void RandomFace_SetEnable(uint8_t enable)
{
    random_face_enabled = enable;
    if (!enable) {
        random_face_state.is_playing_face = 0;
        Face_RequestExpression(FACE_NONE, FACE_PRIORITY_HIGHEST, 1);
    }
}

/**
  * @brief  查询随机表情系统是否启用
  */
uint8_t RandomFace_IsEnabled(void)
{
    return random_face_enabled;
}

/**
  * @brief  获取默认表情概率配置
  */
const RandomFaceConfig* RandomFace_GetDefaultConfig(void)
{
    return &default_config;
}

/* ========== 静态函数实现 ========== */

static FaceType SelectRandomFace(void)
{
    uint8_t total_probability = 0;
    uint8_t i;
    
    for (i = 0; i < random_face_state.config.table_size; i++) {
        total_probability += random_face_state.config.probability_table[i].probability;
    }
    
    uint8_t random_value = GetRandomInRange(0, total_probability - 1);
    
    uint8_t accumulated_prob = 0;
    for (i = 0; i < random_face_state.config.table_size; i++) {
        accumulated_prob += random_face_state.config.probability_table[i].probability;
        if (random_value < accumulated_prob) {
            return random_face_state.config.probability_table[i].face_type;
        }
    }
    
    return random_face_state.config.probability_table[0].face_type;
}

static uint32_t GetRandomInRange(uint32_t min, uint32_t max)
{
    if (min >= max) {
        return min;
    }
    uint32_t range = max - min + 1;
    uint32_t random_value = GetTick() % range;
    return min + random_value;
}

static uint8_t ShouldPlayNewFace(void)
{
    uint32_t current_time = GetTick();
    uint32_t time_since_last_face = current_time - random_face_state.last_face_time;
    
    if (time_since_last_face < random_face_state.config.min_interval) {
        return 0;
    }
    if (time_since_last_face >= random_face_state.config.max_interval) {
        return 1;
    }
    
    uint32_t decision_window = random_face_state.config.max_interval - random_face_state.config.min_interval;
    uint32_t time_in_window = time_since_last_face - random_face_state.config.min_interval;
    uint32_t probability = (time_in_window * 100) / decision_window;
    uint32_t random_value = GetRandomInRange(0, 100);
    
    return (random_value < probability);
}

static void PlayRandomFace(void)
{
    FaceType selected_face = SelectRandomFace();
    
    uint8_t i;
    const FaceProbability* face_config = NULL;
    
    for (i = 0; i < random_face_state.config.table_size; i++) {
        if (random_face_state.config.probability_table[i].face_type == selected_face) {
            face_config = &random_face_state.config.probability_table[i];
            break;
        }
    }
    
    if (face_config == NULL) {
        face_config = &default_probability_table[0];
    }
    
    uint32_t duration = GetRandomInRange(face_config->min_duration, face_config->max_duration);
    Face_RequestExpression(selected_face, FACE_PRIORITY_LOW, 0);
    
    random_face_state.is_playing_face = 1;
    random_face_state.current_face_end_time = GetTick() + duration;
}
