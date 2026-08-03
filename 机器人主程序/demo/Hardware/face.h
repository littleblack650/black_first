/**
  ******************************************************************************
  * @file    face.h
  * @author  生活中的小黑
  * @version V 2.0.0
  * @date    2025年8月4日
  * @brief   表情系统模块头文件
  ******************************************************************************
  */

#ifndef _FACE_H_
#define _FACE_H_

#include "stm32f10x.h"
#include "usart.h"

/* 表情类型枚举 - 定义所有支持的表情 */
typedef enum {
    FACE_NONE = 0,          // 无表情
    FACE_BLINK,             // 眨眼
    FACE_SMILE,             // 微笑
    FACE_CRY,               // 哭
    FACE_SAD,               // 悲伤
    FACE_STRUGGLE,          // 挣扎
    FACE_ANGRY,             // 生气
    FACE_COMFORTABLE,       // 舒适
    FACE_EAT,               // 吃东西
    FACE_LEFT,              // 左看
    FACE_RIGHT,             // 右看
    FACE_UP,                // 上看
    FACE_DOWN,              // 下看
    FACE_LEFTUP,            // 左上
    FACE_RIGHTUP,           // 右上
    FACE_LEFTDOWN,          // 左下
    FACE_RIGHTDOWN,         // 右下
    FACE_SHY,               // 害羞
    FACE_DOUBT,             // 疑惑
    FACE_DIZZY,             // 头晕
    FACE_VOMIT,             // 呕吐
    FACE_LOVE,              // 爱心
    FACE_WAKE,              // 唤醒
    FACE_SLEEP,             // 睡眠
    FACE_RESPOND1,          // 响应1
    FACE_BT,                // 泰坦陨落
    FACE_RESPOND3,          // 响应3
    
    /* 扩展表情 - 与语音指令对应 */
    FACE_DANCE,             // 跳舞
    FACE_ANGRY1,            // 生气1
    FACE_ANGRY2,            // 生气2
    FACE_ANGRY3,            // 生气3
    FACE_BAINIAN,           // 拜年
    FACE_KEBI,              // 科比
    FACE_KEBISING,          // 科比唱歌
	
    FACE_COUNT              // 表情总数，必须为最后一个
} FaceType;

/* 动画帧结构 - 定义一帧的Flash地址和显示时间 */
typedef struct {
    uint32_t addr;      // 图片在Flash中的地址
    uint32_t delay_ms;  // 该帧显示时间(毫秒)
} AnimationFrame;

/* 单帧表情时长配置 - 用于没有动画序列的表情 */
typedef struct {
    FaceType face_type;
    uint32_t display_time;
} SingleFrameTimeConfig;

/* 表情配置结构 - 定义一个表情的所有属性 */
typedef struct {
    FaceType face_type;                 // 表情类型
    const AnimationFrame* sequence;     // 动画序列指针
    uint8_t sequence_length;            // 序列长度
    uint8_t loop_count;                 // 循环次数 (0=无限循环)
    uint32_t pre_delay_min;             // 预延迟最小值(ms)
    uint32_t pre_delay_max;             // 预延迟最大值(ms)
    uint8_t has_special_behavior;       // 是否有特殊行为(如眨眼)
} FaceConfig;

/* 表情运行状态结构 - 记录当前播放进度 */
typedef struct {
    FaceType current_face;              // 当前正在播放的表情
    const FaceConfig* current_config;   // 当前表情的配置
    uint8_t state;                      // 内部状态机状态
    uint32_t last_update_time;          // 上次更新时间 (tick)
    uint32_t random_delay;              // 当前帧的随机延迟时间
    uint8_t loop_count;                 // 已循环次数
    uint8_t frame_index;                // 当前播放到第几帧
    /* 动态flash序列播放字段 */
    uint32_t dynamic_base_addr;         // 起始地址
    uint16_t dynamic_frame_count;       // 总帧数
    uint32_t dynamic_frame_stride;      // 帧间隔(字节)
    uint32_t dynamic_frame_interval;    // 每帧显示时间(ms)
    uint16_t dynamic_current_frame;     // 当前帧索引
    uint8_t dynamic_active;             // 是否正在播放动态序列
    uint8_t dynamic_loop;               // 是否循环播放
    uint8_t current_priority;           // 当前表情优先级（0=无）
} FaceState;

/* 函数声明 */

/**
  * @brief  初始化表情系统
  * @param  无
  * @retval 无
  */
void Face_Init(void);

/**
  * @brief  设置当前表情（兼容旧接口）
  * @param  face_type: 要设置的表情类型
  * @retval 无
  */
void Face_SetExpression(FaceType face_type);

/* 带优先级的表情请求API
 * priority: 数值越高优先级越高；force=1 时绕过优先级检查
 */
void Face_RequestExpression(FaceType face_type, uint8_t priority, uint8_t force);

/* 获取当前表情优先级 */
uint8_t Face_GetCurrentPriority(void);

/**
  * @brief  更新表情 (在主循环中调用)
  * @note   建议每隔10-50ms调用一次，频率不宜过高
  * @param  无
  * @retval 1: 表情更新完成, 0: 表情仍在播放中
  */
uint8_t Face_Update(void);

/**
  * @brief  获取当前正在播放的表情类型
  * @param  无
  * @retval 当前表情类型
  */
FaceType Face_GetCurrentExpression(void);

/**
  * @brief  添加或修改表情配置
  * @param  config: 表情配置结构体指针
  * @retval 无
  * @note   可用于动态修改默认配置或添加新表情
  */
void Face_AddConfig(const FaceConfig* config);

/* 播放Flash中连续存储的帧序列
 * start_addr: 第一帧的Flash地址
 * end_addr: 最后一帧的Flash地址（必须 >= start_addr）
 * frame_interval_ms: 每帧显示时间(ms)
 * loop: 1为无限循环，0为播放一次后停止
 * 注：帧间隔默认为0x1000字节
 */
void Face_PlayFlashSequenceRange(uint32_t start_addr, uint32_t end_addr, uint32_t frame_interval_ms, uint8_t loop);

/* 停止任何正在进行的flash序列播放 */
void Face_StopFlashSequence(void);

/**
  * @brief  进入静态帧模式，固定显示某一帧，停止表情自动更新
  * @param  addr  要显示的图片在Flash中的地址
  */
void Face_EnterStaticMode(uint32_t addr);

/**
  * @brief  退出静态帧模式，恢复表情系统正常运行
  */
void Face_ExitStaticMode(void);

/**
  * @brief  检查当前是否处于静态帧模式
  */
uint8_t Face_IsStaticMode(void);

/**
  * @brief  获取默认表情配置表
  * @param  无
  * @retval 默认配置表指针
  * @note   可用于参考或通过Face_AddConfig修改
  */
const FaceConfig* Face_GetDefaultConfigs(void);

/**
  * @brief  获取默认表情配置数量
  * @param  无
  * @retval 默认配置表的大小
  */
uint8_t Face_GetDefaultConfigCount(void);

/* 语音指令到表情类型的转换函数 */
FaceType VoiceCommand_To_FaceType(VoiceCommand cmd);

/* 公开的优先级等级 */
#define FACE_PRIORITY_LOW      1
#define FACE_PRIORITY_MEDIUM   3
#define FACE_PRIORITY_HIGH     5
#define FACE_PRIORITY_HIGHEST  7

#endif
