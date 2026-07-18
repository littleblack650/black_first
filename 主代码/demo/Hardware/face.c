/**
  ******************************************************************************
  * @file    face.c
  * @author  生活中的小黑
  * @version V 2.0.0
  * @date    2025年8月4日
  * @brief   表情系统模块 - 优化版本
  ******************************************************************************
  */

#include "face.h"
#include "Delay.h"
#include "OLED.h"
#include "W25Q64.h"
#include "random.h"
#include "pwm.h"
#include "usart.h"

/* 全局变量 */

/**
  * @brief  表情状态结构体
  */
static FaceState face_state = {FACE_NONE};
static uint8_t static_mode = 0;
static uint32_t static_frame_addr = 0;
/**
  * @brief  显示缓冲区 (放在CCM RAM中)
  */
static u8 readBuffer1[1024] __attribute__((section(".ccmram")));

/**
  * @brief  眼睛图形地址表
  */
static const uint32_t eyeTab[] = {0x000000, 0x004000, 0x005000};
static const uint8_t EYE_TAB_SIZE = sizeof(eyeTab) / sizeof(eyeTab[0]);
static uint8_t currentEyeIndex = 0;

/**
  * @brief  表情配置数组
  * @note   存储所有表情的配置，可通过Face_AddConfig动态修改
  */
static FaceConfig face_configs[FACE_COUNT] = {FACE_NONE};

/* 预定义动画序列 - 所有序列数组均保持不变（只是注释乱码已修正） */

static const AnimationFrame blink_sequence[] = {
    {0x001000, 50}, {0x002000, 50}, {0x003000, 50},
    {0x006000, 50}, {0x003000, 50}, {0x002000, 50}, {0x001000, 50}
};

static const AnimationFrame smile_sequence[] = {
    {0x007000, 2000}, {0x008000, 2000}
};

static const AnimationFrame cry_sequence[] = {
    {0x009000, 2000}, {0x006000, 500}, {0x009000, 1000}, {0x006000, 200}
};

static const AnimationFrame sad_sequence[] = {
    {0x00A000, 150}, {0x00B000, 150}, {0x00C000, 2000},
    {0x006000, 200}, {0x00C000, 2000}, {0x006000, 200}, {0x00C000, 1000}
};

static const AnimationFrame angry_sequence[] = {
    {0x00E000, 100}, {0x00F000, 100}, {0x010000, 2000},
    {0x006000, 100}, {0x010000, 2000}, {0x006000, 100},
	{0x010000, 1000},{0x006000, 200}, {0x010000, 1000},
	{0x006000, 100}, {0x010000, 1000},
};

static const AnimationFrame comfortable_sequence[] = {
    {0x01F000, 100}, {0x020000, 100}, {0x011000, 1000},
    {0x012000, 2000}, {0x013000, 100}, {0x014000, 1000}
};

static const AnimationFrame eat_sequence[] = {
    {0x015000, 300}, {0x016000, 300}, {0x017000, 300},
    {0x018000, 1000}
};

static const AnimationFrame left_sequence[] = {
    {0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x01B000, 50}, {0x01A000, 50},
    {0x019000, 3000}, {0x01A000, 50}, {0x01B000, 50},
    {0x006000, 50}, 
};

static const AnimationFrame right_sequence[] = {
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x01E000, 50}, {0x01C000, 50},
    {0x01D000, 3000}, {0x01C000, 50}, {0x01E000, 50},
    {0x006000, 50}, 
};

static const AnimationFrame leftup_sequence[] = {
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x025000, 50}, {0x026000, 1000},
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x025000, 50}, {0x026000, 2000},
};

static const AnimationFrame rightup_sequence[] = {
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x028000, 50}, {0x029000, 1000},
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x028000, 50}, {0x029000, 2000}
};

static const AnimationFrame leftdown_sequence[] = {
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x02B000, 50}, {0x02C000, 1000},
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x02B000, 50}, {0x02C000, 2000}
};

static const AnimationFrame rightdown_sequence[] = {
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x02E000, 50}, {0x02F000, 1000},
	{0x001000, 50}, {0x002000, 50}, {0x003000, 50},
	{0x006000, 50}, {0x02E000, 50}, {0x02F000, 2000}
};

static const AnimationFrame doubt_sequence[] = {
    {0x002000, 200}, {0x006000, 500}, {0x002000, 200}
};

static const AnimationFrame dizzy_sequence[] = {
    {0x035000, 50}, {0x036000, 50}, {0x037000, 50}, {0x038000, 50},
    {0x039000, 50}, {0x03A000, 50}, {0x03B000, 50}, {0x03C000, 50},
    {0x035000, 50}, {0x036000, 50}, {0x037000, 50}, {0x038000, 50},
    {0x039000, 50}, {0x03A000, 50}, {0x03B000, 50}, {0x03C000, 50}
};

static const AnimationFrame vomit_sequence[] = {
    {0x03D000, 100}, {0x03E000, 100}, {0x03F000, 100}, {0x040000, 100},
	{0x03D000, 100}, {0x03E000, 100}, {0x03F000, 100}, {0x040000, 100},
	{0x03D000, 100}, {0x03E000, 100}, {0x03F000, 100}, {0x040000, 100}
};

static const AnimationFrame love_sequence[] = {
    {0x032000, 4000}, {0x033000, 4000}
};

static const AnimationFrame wake_sequence[] = {
    {0x041000, 2000}, {0x042000, 500}, {0x043000, 1000},
    {0x042000, 100}, {0x041000, 100}, {0x042000, 50},
    {0x043000, 50}, {0x044000, 50}, {0x004000, 2000}
};

static const AnimationFrame sleep_sequence[] = {
    {0x044000, 1000}, {0x043000, 500}, {0x042000, 1000},
    {0x041000, 1000}, {0x042000, 500}, {0x042000, 1000}, {0x041000, 2000}
};
static const AnimationFrame struggle_sequence[] = {
	{0x00D000, 4000}
};
static const AnimationFrame up_sequence[] = {
    {0x000000, 100}, {0x01F000, 100}, {0x020000, 1000},
    {0x021000, 100}, {0x020000, 1000}, {0x01F000, 100}, {0x000000, 100}
};
static const AnimationFrame down_sequence[] = {
    {0x000000, 100}, {0x022000, 100}, {0x023000, 1000},
    {0x024000, 100}, {0x023000, 1000}, {0x022000, 100}, {0x000000, 100}
};
static const AnimationFrame bt_sequence[] = {
    {0x046000, 200}, {0x047000, 200}, {0x048000, 200},
    {0x049000, 200}, {0x04A000, 500}, {0x049000, 100}, {0x04A000, 100},
	{0x049000, 200}, {0x04A000, 500}, {0x049000, 100}, {0x04A000, 100}
};
static const AnimationFrame sing_sequence[] = {
    {0x04D000, 11000}, {0x04B000, 1000}, {0x04C000, 1000},
    {0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04C000, 1000},
	{0x04D000, 1000}, {0x04B000, 1000}, {0x04B000, 10000},
};
static const AnimationFrame dance_sequence[] = {
    {0x008000, 8000}   
};
/* 默认表情配置表 - 所有表情都配置了动画序列 */
static const FaceConfig default_face_configs[] = {
    // 表情类型        动画序列            序列长度 循环次数 最小预延迟 最大预延迟 特殊行为
    {FACE_NONE,        NULL,                  0,        0,      0,         0,         0},
    {FACE_BLINK,       blink_sequence,        7,        4,      2000,      3000,      0},
    {FACE_SMILE,       smile_sequence,        2,        1,      0,         0,         0},
    {FACE_CRY,         cry_sequence,          4,        1,      0,         0,         0},
    {FACE_SAD,         sad_sequence,          7,        1,      0,         0,         0},
    {FACE_STRUGGLE,    struggle_sequence,     1,        1,      0,         0,         0},
	{FACE_ANGRY,       angry_sequence,        11,       1,      0,         0,         0}, 
    {FACE_COMFORTABLE, comfortable_sequence,  6,        1,      0,         0,         0},
    {FACE_EAT,         eat_sequence,          4,        4,      0,         0,         0},
    {FACE_LEFT,        left_sequence,         10,       1,      0,         0,         0},
    {FACE_RIGHT,       right_sequence,        10,       1,      0,         0,         0},
    {FACE_UP,          up_sequence,           1,        1,      0,         0,         0},
    {FACE_DOWN,        down_sequence,         1,        1,      0,         0,         0},
    {FACE_LEFTUP,      leftup_sequence,       12,       1,      0,         0,         0}, 
    {FACE_RIGHTUP,     rightup_sequence,      12,       1,      0,         0,         0},
    {FACE_LEFTDOWN,    leftdown_sequence,     12,       1,      0,         0,         0}, 
    {FACE_RIGHTDOWN,   rightdown_sequence,    12,       1,      0,         0,         0}, 
    {FACE_SHY,         NULL,                  0,        1,      0,         0,         0}, 
    {FACE_DOUBT,       doubt_sequence,        3,        4,      0,         0,         0},
    {FACE_DIZZY,       dizzy_sequence,        16,       1,      0,         0,         0},
    {FACE_VOMIT,       vomit_sequence,        12,       3,      0,         0,         0},
    {FACE_LOVE,        love_sequence,         2,        1,      0,         0,         0},
    {FACE_WAKE,        wake_sequence,         9,        1,      0,         0,         0},
    {FACE_SLEEP,       sleep_sequence,        7,        1,      0,         0,         0},
    {FACE_RESPOND1,    NULL,        		  0,        1,      0,         0,         0},
    {FACE_BT,  		   bt_sequence,           11,       1,      0,         0,         0},
    {FACE_RESPOND3,    NULL,                  0,        2,      0,         0,         0},
	{FACE_DANCE,       dance_sequence,        1,        1,      0,         0,         0},
    {FACE_ANGRY1,      NULL,                  0,        1,      0,         0,         0},
    {FACE_ANGRY2,      NULL,                  0,        1,      0,         0,         0},
    {FACE_ANGRY3,      NULL,                  0,        1,      0,         0,         0},
    {FACE_BAINIAN,     NULL,                  0,        1,      0,         0,         0},
    {FACE_KEBI,        NULL,                  0,        1,      0,         0,         0},
    {FACE_KEBISING,    sing_sequence,         51,       1,      0,         0,         0},
};

/* 单帧表情配置 - 用于只有单帧动画的表情 */
static const SingleFrameTimeConfig single_frame_times[] = {
    {FACE_STRUGGLE,   3000},   
    {FACE_SHY,        2000},   
    {FACE_ANGRY3,     5000},
    {FACE_KEBI,       2000},   
    {FACE_ANGRY1,     2000},   
    {FACE_ANGRY2,     2000}, 
};

#define SINGLE_FRAME_TIME_COUNT (sizeof(single_frame_times) / sizeof(single_frame_times[0]))

/* 默认每帧在 flash 中的地址间隔（字节） */
#define DEFAULT_FRAME_STRIDE 0x1000
	
/* 内部函数声明 */
static void DisplayFrame(uint32_t addr);
static uint32_t GetRandomDelay(uint32_t min, uint32_t max);
static void ResetFaceState(FaceType new_face);
static uint8_t UpdateSimpleFace(void);
static uint8_t UpdateSpecialFace(void);
static uint8_t UpdateBlinkSpecial(void);
static uint8_t UpdateRespondWithSequence(FaceType face_type);
static uint32_t GetSingleFrameAddress(FaceType face_type);
static uint8_t UpdateDynamicSequence(void);

/* 表情优先级等级 */
#define FACE_PRIORITY_LOW      1
#define FACE_PRIORITY_MEDIUM   3
#define FACE_PRIORITY_HIGH     5
#define FACE_PRIORITY_HIGHEST  7

/* 新API */
void Face_RequestExpression(FaceType face_type, uint8_t priority, uint8_t force);
uint8_t Face_GetCurrentPriority(void) { return face_state.current_priority; }

/**
  * @brief  初始化表情系统
  */
void Face_Init(void)
{
    uint8_t i;
    
    // 拷贝默认配置
    for (i = 0; i < FACE_COUNT; i++) {
        face_configs[i] = default_face_configs[i];
    }
    
    ResetFaceState(FACE_NONE);
    currentEyeIndex = 0;
}

/**
  * @brief  设置当前表情（兼容旧接口）
  */
void Face_SetExpression(FaceType face_type)
{
    /* 向后兼容包装：默认使用中优先级，不强制 */
    Face_RequestExpression(face_type, FACE_PRIORITY_MEDIUM, 0);
}

/**
 * Face_RequestExpression: 请求切换表情，带优先级仲裁
 * priority: 数值越高优先级越高；force=1 时绕过所有检查
 */
void Face_RequestExpression(FaceType face_type, uint8_t priority, uint8_t force)
{
    if (face_type >= FACE_COUNT) return;

	if (face_type >= FACE_COUNT) return;

    /* ========== 新增：静态模式处理 ========== */
    if (static_mode && !force) {
        return;   // 静态模式下忽略非强制表情请求
    }
    if (static_mode && force) {
        Face_ExitStaticMode();   // 强制请求会退出静态模式
    }
	
    /* 如果已经是相同表情，若强制或优先级更高则更新优先级 */
    if (face_state.current_face == face_type) {
        if (force || priority > face_state.current_priority) {
            face_state.current_priority = (face_type == FACE_NONE) ? 0 : priority;
        }
        return;
    }

    /* 如果正在播放动态flash序列且未强制，只允许 NONE 或最高优先级中断 */
    if (!force && face_state.dynamic_active && face_type != FACE_NONE && priority < FACE_PRIORITY_HIGHEST) {
        return;
    }

    /* 优先级检查：除非强制或新优先级更高，否则不抢占 */
    if (!force && face_state.current_priority != 0 && priority < face_state.current_priority) {
        return;
    }

    /* 应用新表情 */
    ResetFaceState(face_type);
    face_state.current_config = &face_configs[face_type];
    face_state.current_priority = (face_type == FACE_NONE) ? 0 : priority;

#ifdef DEBUG_ENABLE
    if (face_type == FACE_KEBI || face_type == FACE_KEBISING) {
        printf("Face_RequestExpression: requested face=%d priority=%d force=%d\n", face_type, priority, force);
    }
#endif

    /* 特殊处理：RESPOND3 使用动态flash播放 */
    if (face_type == FACE_RESPOND3) {
        Face_PlayFlashSequenceRange(0x04E000, 0x0B1000, 100, 0);
        /* 确保 RESPOND3 播放期间具有最高优先级 */
        face_state.current_priority = FACE_PRIORITY_HIGHEST;
    }
}

/**
  * @brief  获取当前表情类型
  */
FaceType Face_GetCurrentExpression(void)
{
    return face_state.current_face;
}

/**
  * @brief  更新表情 - 在主循环中调用
  */
uint8_t Face_Update(void)
{
    // 静态模式下不进行任何更新，只维持当前显示
    if (static_mode) {
        return 1;   // 返回1表示“完成”，避免外部逻辑误判
    }
	
    uint32_t current_time = GetTick();
    // 动态 flash 序列优先级更高
    if (face_state.dynamic_active) {
        if (current_time - face_state.last_update_time < face_state.random_delay) {
            return 0;
        }
        face_state.last_update_time = current_time;
        return UpdateDynamicSequence();
    }

    if (face_state.current_face == FACE_NONE) {
        return 1;
    }

    if (current_time - face_state.last_update_time < face_state.random_delay) {
        return 0;
    }

    face_state.last_update_time = current_time;

    // 根据是否有特殊行为分别处理
    if (face_state.current_config->has_special_behavior) {
        return UpdateSpecialFace();
    } else {
        return UpdateSimpleFace();
    }
}

/**
  * @brief  添加或修改表情配置
  */
void Face_AddConfig(const FaceConfig* config)
{
    if (config == NULL || config->face_type >= FACE_COUNT) {
        return;
    }
    
    face_configs[config->face_type] = *config;
}

/**
  * @brief  获取默认表情配置表
  */
const FaceConfig* Face_GetDefaultConfigs(void)
{
    return default_face_configs;
}

/**
  * @brief  获取默认表情配置数量
  */
uint8_t Face_GetDefaultConfigCount(void)
{
    return sizeof(default_face_configs) / sizeof(default_face_configs[0]);
}

/**
  * @brief  获取单帧表情的显示时间
  * @param  face_type: 表情类型
  * @retval 显示时间(毫秒)，若未配置返回2000ms
  */
static uint32_t GetSingleFrameDisplayTime(FaceType face_type)
{
    for (uint8_t i = 0; i < SINGLE_FRAME_TIME_COUNT; i++) {
        if (single_frame_times[i].face_type == face_type) {
            return single_frame_times[i].display_time;
        }
    }
    return 2000; // 默认2秒
}

/* 内部函数实现 */

/**
  * @brief  更新普通表情 - 根据配置序列播放
  */
static uint8_t UpdateSimpleFace(void)
{
    const FaceConfig* config = face_state.current_config;
    
    switch (face_state.state) {
        case 0: /* 预延迟状态 - 播放眼睛等待 */
            if (config->pre_delay_max > 0) {
                // 播放随机眼睛表情
                DisplayFrame(eyeTab[currentEyeIndex]);
                currentEyeIndex = (currentEyeIndex + 1) % EYE_TAB_SIZE;
                face_state.random_delay = GetRandomDelay(config->pre_delay_min, config->pre_delay_max);
                face_state.state = 1;
            } else {
                // 无预延迟，直接开始播放序列
                face_state.frame_index = 0;
                face_state.state = 2;
            }
            return 0;
            
        case 1: /* 预延迟结束，准备播放序列 */
            face_state.frame_index = 0;
            face_state.state = 2;
            return 0;
            
        case 2: /* 播放动画序列或单帧 */
            if (config->sequence == NULL) {
                // 单帧表情（如STRUGGLE, SHY, ANGRY1等）
                DisplayFrame(GetSingleFrameAddress(face_state.current_face));
                
                // 获取显示时长
                uint32_t display_time = GetSingleFrameDisplayTime(face_state.current_face);
                
                if (display_time > 0) {
                    // 设置延迟
                    face_state.random_delay = display_time;
                    face_state.state = 3; // 进入显示完成状态
                } else {
                    // 显示时间为0，立即结束
                    ResetFaceState(FACE_NONE);
                    return 1;
                }
                return 0;
            }
            
            // 播放序列帧
            if (face_state.frame_index < config->sequence_length) {
                // 显示当前帧
                DisplayFrame(config->sequence[face_state.frame_index].addr);
                face_state.random_delay = config->sequence[face_state.frame_index].delay_ms;
                face_state.frame_index++;
                return 0;
            }
            
            // 序列播放完成，处理循环
            face_state.loop_count++;
            if (config->loop_count == 0 || face_state.loop_count < config->loop_count) {
                // 继续循环
                face_state.frame_index = 0;
                if (config->pre_delay_max > 0) {
                    face_state.state = 0; // 重新进入预延迟
                }
                return 0;
            }
            
            // 所有循环完成，结束表情
            ResetFaceState(FACE_NONE);
            return 1;
            
        case 3: /* 单帧表情显示等待状态 - 直接结束 */
            ResetFaceState(FACE_NONE);
            return 1;
            
        default:
            ResetFaceState(FACE_NONE);
            return 1;
    }
}

/**
  * @brief  更新特殊表情 - 如眨眼等有特殊行为
  */
static uint8_t UpdateSpecialFace(void)
{
    switch (face_state.current_face) {
        case FACE_BLINK:
            return UpdateBlinkSpecial();
        default:
            // 其他特殊表情暂未实现，按普通处理
            ResetFaceState(FACE_NONE);
            return 1;
    }
}

/**
  * @brief  更新眨眼表情
  */
static uint8_t UpdateBlinkSpecial(void)
{
    switch (face_state.state) {
        case 0: /* 随机眼睛表情 */
            DisplayFrame(eyeTab[currentEyeIndex]);
            currentEyeIndex = (currentEyeIndex + 1) % EYE_TAB_SIZE;
            face_state.random_delay = GetRandomDelay(face_state.current_config->pre_delay_min, 
                                                   face_state.current_config->pre_delay_max);
            face_state.state = 1;
            return 0;
            
        case 1: /* 预延迟结束，准备眨眼 */
            face_state.frame_index = 0;
            face_state.state = 2;
            return 0;
            
        case 2: /* 播放眨眼序列 */
            if (face_state.frame_index < face_state.current_config->sequence_length) {
                DisplayFrame(face_state.current_config->sequence[face_state.frame_index].addr);
                face_state.random_delay = face_state.current_config->sequence[face_state.frame_index].delay_ms;
                face_state.frame_index++;
                return 0;
            }
            
            // 眨眼完成
            face_state.loop_count++;
            if (face_state.current_config->loop_count == 0 || 
                face_state.loop_count < face_state.current_config->loop_count) {
                // 继续循环
                face_state.state = 0;
                return 0;
            }
            
            // 结束
            ResetFaceState(FACE_NONE);
            return 1;
            
        default:
            ResetFaceState(FACE_NONE);
            return 1;
    }
}

/* 语音指令到表情的映射函数 */
FaceType VoiceCommand_To_FaceType(VoiceCommand cmd)
{
    switch (cmd) {
        case VOICE_CMD_01: return FACE_RESPOND1;
        case VOICE_CMD_02: return FACE_BT;
        case VOICE_CMD_03: return FACE_STRUGGLE;
        case VOICE_CMD_04: return FACE_SMILE;
        case VOICE_CMD_05: return FACE_RESPOND1;
        case VOICE_CMD_06: return FACE_RESPOND1;
        case VOICE_CMD_07: return FACE_RESPOND1;
        case VOICE_CMD_08: return FACE_RESPOND1;
        case VOICE_CMD_09: return FACE_RESPOND1;
        case VOICE_CMD_0A: return FACE_RESPOND1;
        case VOICE_CMD_0B: return FACE_RESPOND1;
        case VOICE_CMD_0C: return FACE_DANCE;
        case VOICE_CMD_0D: return FACE_LOVE;
        case VOICE_CMD_0E: return FACE_UP;
        case VOICE_CMD_0F: return FACE_ANGRY1;
        case VOICE_CMD_11: return FACE_ANGRY2;
        case VOICE_CMD_12: return FACE_ANGRY3;
        case VOICE_CMD_13: return FACE_CRY;
        case VOICE_CMD_14: return FACE_SAD;
        case VOICE_CMD_16: return FACE_DOUBT;
        case VOICE_CMD_17: return FACE_EAT;
        case VOICE_CMD_18: return FACE_VOMIT;
        case VOICE_CMD_19: return FACE_ANGRY;
        case VOICE_CMD_1A: return FACE_DOWN;
        case VOICE_CMD_1B: return FACE_LEFT;
        case VOICE_CMD_1C: return FACE_RIGHT;
        case VOICE_CMD_1D: return FACE_BAINIAN;
        case VOICE_CMD_1E: return FACE_SLEEP; 
        case VOICE_CMD_1F: return FACE_WAKE; 
        case VOICE_CMD_20: return FACE_KEBI;
        case VOICE_CMD_21: return FACE_KEBISING;  // 唱歌
		case VOICE_CMD_22: return FACE_RESPOND1;  // 响应
		case VOICE_CMD_23: return FACE_RESPOND3;
        default: return FACE_NONE;
    }
}

/**
  * @brief  获取单帧表情的Flash地址
  */
static uint32_t GetSingleFrameAddress(FaceType face_type)
{
    uint32_t addr = 0x006000; // default
    switch (face_type) {
        case FACE_RESPOND1: addr = 0x004000; break;
        case FACE_STRUGGLE: addr = 0x00D000; break;
        case FACE_SHY:      addr = 0x031000; break;
        case FACE_ANGRY3:   addr = 0x0B2000; break;
        case FACE_KEBI:     addr = 0x045000; break;
        case FACE_ANGRY1:   addr = 0x00E000; break;
        case FACE_ANGRY2:   addr = 0x00F000; break;
        default:            addr = 0x006000; break;
    }
#ifdef DEBUG_ENABLE
    printf("GetSingleFrameAddress: face=%d -> addr=0x%06X\n", face_type, addr);
#endif
    return addr;
}

/**
  * @brief  显示一帧图像
  */
static void DisplayFrame(uint32_t addr)
{
    uint8_t ret;
    
    // 尝试读取指定地址的图片
    ret = W25Q64_ReadData(addr, readBuffer1, sizeof(readBuffer1));
    if (ret == 0) {
        // 读取成功，正常显示
        OLED_ShowPicture(0, 0, 128, 64, readBuffer1, 1);
    } else {
        // 读取失败，显示备用图片（地址 0x003000）
        ret = W25Q64_ReadData(0x003000, readBuffer1, sizeof(readBuffer1));
        if (ret == 0) {
            OLED_ShowPicture(0, 0, 128, 64, readBuffer1, 1);
        } else {
            // 备用图片也读取失败：可以选择不更新屏幕（保持上次显示）或显示内置图案
            // 这里选择不更新
        }
    }
}

/**
  * @brief  获取随机延迟时间
  */
static uint32_t GetRandomDelay(uint32_t min, uint32_t max)
{
    return min + (GetTick() % (max - min));
}

/**
  * @brief  重置表情状态
  */
static void ResetFaceState(FaceType new_face)
{
    face_state.current_face = new_face;
    face_state.current_config = (new_face == FACE_NONE) ? NULL : &face_configs[new_face];
    face_state.state = 0;
    face_state.last_update_time = GetTick();
    face_state.random_delay = 0;
    face_state.loop_count = 0;
    face_state.frame_index = 0;
    /* 清除动态播放状态 */
    face_state.dynamic_base_addr = 0;
    face_state.dynamic_frame_count = 0;
    face_state.dynamic_frame_stride = 0;
    face_state.dynamic_frame_interval = 0;
    face_state.dynamic_current_frame = 0;
    face_state.dynamic_active = 0;
    face_state.dynamic_loop = 0;
    /* 清除优先级 */
    face_state.current_priority = 0;
}

/**
  * @brief  更新动态flash序列播放
  * @retval 1: 已完成或已停止， 0: 仍在播放
  */
static uint8_t UpdateDynamicSequence(void)
{
    if (!face_state.dynamic_active || face_state.dynamic_frame_count == 0) {
        face_state.dynamic_active = 0;
        return 1;
    }

    if (face_state.dynamic_current_frame < face_state.dynamic_frame_count) {
        uint32_t addr = face_state.dynamic_base_addr + ((uint32_t)face_state.dynamic_current_frame * face_state.dynamic_frame_stride);
        DisplayFrame(addr);
        face_state.random_delay = face_state.dynamic_frame_interval;
        face_state.dynamic_current_frame++;
        return 0;
    }

    // 到达末尾
    if (face_state.dynamic_loop) {
        face_state.dynamic_current_frame = 0;
        return 0;
    }

    // 停止播放
    face_state.dynamic_active = 0;
    if (face_state.current_priority == FACE_PRIORITY_HIGHEST) {
        face_state.current_priority = 0;
    }
    return 1;
}

/**
  * @brief  开始播放flash中连续存储的帧序列
  */
void Face_PlayFlashSequenceRange(uint32_t start_addr, uint32_t end_addr, uint32_t frame_interval_ms, uint8_t loop)
{
    if (end_addr < start_addr) return;

    uint32_t stride = DEFAULT_FRAME_STRIDE;
    uint32_t total_bytes = end_addr - start_addr;
    uint32_t count = (total_bytes / stride) + 1;
    if (count == 0) return;
    if (count > 0xFFFF) count = 0xFFFF;

    face_state.dynamic_base_addr = start_addr;
    face_state.dynamic_frame_count = (uint16_t)count;
    face_state.dynamic_frame_stride = stride;
    face_state.dynamic_frame_interval = frame_interval_ms;
    face_state.dynamic_current_frame = 0;
    face_state.dynamic_active = 1;
    face_state.dynamic_loop = loop ? 1 : 0;
    face_state.last_update_time = GetTick();
    face_state.random_delay = 0;
    /* 动态播放期间设置最高优先级 */
    face_state.current_priority = FACE_PRIORITY_HIGHEST;
}

/**
  * @brief  停止任何正在进行的flash序列播放
  */
void Face_StopFlashSequence(void)
{
    face_state.dynamic_active = 0;
    if (face_state.current_priority == FACE_PRIORITY_HIGHEST) {
        face_state.current_priority = 0;
    }
}

void Face_EnterStaticMode(uint32_t addr)
{
    static_mode = 1;
    static_frame_addr = addr;
    // 立即显示这一帧
    DisplayFrame(addr);
    // 重置表情状态机，防止干扰
    face_state.current_face = FACE_NONE;
    face_state.dynamic_active = 0;
}

void Face_ExitStaticMode(void)
{
    static_mode = 0;
}

uint8_t Face_IsStaticMode(void)
{
    return static_mode;
}
