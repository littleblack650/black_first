#include "touch.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "face.h"
#include "delay.h"
#include "pwm.h"
#include "mpu6050.h"
#include <stddef.h>

// 全局变量
static volatile uint8_t g_touchEvent = 0;
static volatile uint32_t g_lastTouchTime = 0;
static uint8_t g_isTouching = 0;
static uint8_t g_touchProcessed = 0;
static uint32_t g_touchEndTime = 0;
static uint8_t g_shyActionRunning = 0;
static uint32_t g_lastShyActionTime = 0;
static uint8_t g_lastStableState = 0;      // 上次稳定的触摸状态
static uint32_t g_lastStateChangeTime = 0; // 上次状态变化时间

// 触摸延迟时间配置（完全封装在模块内部）
static uint32_t g_touch_delay_time = 500;  // 默认500ms延迟

// 回调函数指针
static TouchCallback g_touchStartCallback = NULL;
static TouchCallback g_touchEndCallback = NULL;

// 私有函数声明
static void Default_Touch_Start_Handler(void);
static void Default_Touch_End_Handler(void);
static void Start_Shy_Action(void);

// 防抖和超时参数
#define TOUCH_DEBOUNCE_MS 50            // 去抖时间（ms）
#define TOUCH_LOCKOUT_MS 300            // 触发后短暂锁定避免重复触发（ms）
#define TOUCH_RELEASE_CONFIRM_COUNT 10  // 措施1：连续确认释放所需次数（每次10ms共100ms）
#define TOUCH_WATCHDOG_TIMEOUT_MS 5000  // 措施2：触摸看门狗超时时间（5秒强制释放）
#define SHY_ACTION_INTERVAL 300     // 害羞动作序列执行间隔(ms)

// 默认触摸开始处理函数
static void Default_Touch_Start_Handler(void)
{
    // 触发害羞表情
    Face_RequestExpression(FACE_SHY, FACE_PRIORITY_HIGH, 0);
    
    // 启动害羞动作序列
    Start_Shy_Action();
}

// 默认触摸结束处理函数
static void Default_Touch_End_Handler(void)
{
    // 恢复正常状态
    Face_RequestExpression(FACE_NONE, FACE_PRIORITY_HIGHEST, 1);
    
    // 停止害羞动作
    g_shyActionRunning = 0;
}

// 启动害羞动作序列
static void Start_Shy_Action(void)
{
    // 害羞动作序列
    ServoSequenceCommand sequence[] = {
        {SERVO_CMD_MOVE, 3, 110, 500},  // 头部保持
        {SERVO_CMD_MOVE, 2, 60, 500},   // 轻微转头
        {SERVO_CMD_DELAY, 0, 0, 500},   // 保持1秒
        {SERVO_CMD_MOVE, 2, 120, 500},  // 头转回
        {SERVO_CMD_DELAY, 0, 0, 500},   // 保持1秒
        {SERVO_CMD_MOVE, 2, 60, 500},   // 头转回
        {SERVO_CMD_DELAY, 0, 0, 500},   // 保持1秒
        {SERVO_CMD_MOVE, 2, 120, 500},  // 头转回
        {SERVO_CMD_MOVE, 3, 90, 500},   // 头转回
        {SERVO_CMD_MOVE, 2, 90, 500},   // 头转回
    };
    /* 忽略MPU一段时间，避免随后的舵机动作被误判为摇晃 */
    MPU6050_IgnoreActionsFor(2500);
    Servo_ExecuteSequence(10, sequence);
    
    // 标记害羞动作正在运行
    g_shyActionRunning = 1;
    g_lastShyActionTime = GetTick();
}

// 触摸按键初始化 - 修改为PA10 (上拉输入)
void Touch_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 2. 配置PA10为上拉输入模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 初始化触摸状态
    g_isTouching = 0;
    g_touchProcessed = 0;
    g_lastTouchTime = GetTick();
    g_touchEndTime = 0;
    g_shyActionRunning = 0;
    g_lastShyActionTime = 0;
    g_lastStableState = 0;
    g_lastStateChangeTime = GetTick();
    
    // 设置默认回调函数（害羞表情）
    g_touchStartCallback = Default_Touch_Start_Handler;
    g_touchEndCallback = Default_Touch_End_Handler;
}

// 内部函数：设置延迟时间（模块内部使用）
static void Set_Touch_Delay_Time(uint32_t delay_time)
{
    g_touch_delay_time = delay_time;
}

// 内部函数：获取延迟时间
static uint32_t Get_Touch_Delay_Time(void)
{
    return g_touch_delay_time;
}

// 注册触摸开始回调函数
void Touch_Register_Start_Callback(TouchCallback callback)
{
    if (callback != NULL) {
        g_touchStartCallback = callback;
    }
}

// 注册触摸结束回调函数
void Touch_Register_End_Callback(TouchCallback callback)
{
    if (callback != NULL) {
        g_touchEndCallback = callback;
    }
}

// 获取触摸引脚当前状态 - PA10
uint8_t Get_Touch_State(void)
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
}

// 获取当前触摸状态
uint8_t Is_Touching(void)
{
    return g_isTouching;
}

// 检查是否允许执行其他动作
uint8_t Is_Allow_Other_Actions(void)
{
    uint32_t currentTime = GetTick();
    
    // 如果正在触摸，不允许其他动作
    if (g_isTouching) {
        return 0;
    }
    
    // 如果触摸已结束，检查是否超过延迟时间
    if (g_touchEndTime > 0) {
        uint32_t timeSinceTouchEnd = currentTime - g_touchEndTime;
        if (timeSinceTouchEnd <= g_touch_delay_time) {
            return 0; // 还在延迟期内，不允许其他动作
        }
    }
    
    return 1; // 允许其他动作
}

// 触摸状态机枚举
typedef enum {
    TOUCH_IDLE,        // 空闲状态（未触摸）
    TOUCH_PRESS_WAIT,  // 检测到可能按下，等待消抖
    TOUCH_ACTIVE,      // 已确认触摸（激活态）
    TOUCH_RELEASE_WAIT // 检测到可能释放，等待消抖
} TouchFSMState;

static TouchFSMState touch_fsm_state = TOUCH_IDLE;
static uint32_t touch_debounce_timer = 0;
#define TOUCH_DEBOUNCE_MS   50    // 消抖时间 50ms

/**
  * @brief  触摸处理函数（状态机消抖版）
  * @note   需在主循环中每隔 10-20ms 调用一次
  */
void Touch_Process(void)
{
    uint8_t raw_state = Get_Touch_State();   // 读取原始电平（1：触摸，0：未触摸）
    uint32_t now = GetTick();

    switch (touch_fsm_state)
    {
        case TOUCH_IDLE:
            if (raw_state == 1) {
                // 第一次检测到触摸，进入等待消抖状态
                touch_fsm_state = TOUCH_PRESS_WAIT;
                touch_debounce_timer = now;
            }
            break;

        case TOUCH_PRESS_WAIT:
            if (raw_state == 1) {
                // 持续为触摸态，检查是否达到消抖时间
                if ((now - touch_debounce_timer) >= TOUCH_DEBOUNCE_MS) {
                    // ? 确认触摸开始
                    g_isTouching = 1;
                    g_touchEvent = 1;
                    g_touchProcessed = 0;
                    g_touchEndTime = 0;
                    if (g_touchStartCallback != NULL) {
                        g_touchStartCallback();
                    }
                    touch_fsm_state = TOUCH_ACTIVE;
                }
            } else {
                // 消抖期间电平消失，认定为抖动，回到空闲
                touch_fsm_state = TOUCH_IDLE;
            }
            break;

        case TOUCH_ACTIVE:
            if (raw_state == 0) {
                // 检测到可能释放，进入释放消抖状态
                touch_fsm_state = TOUCH_RELEASE_WAIT;
                touch_debounce_timer = now;
            }
            break;

        case TOUCH_RELEASE_WAIT:
            if (raw_state == 0) {
                // 持续为未触摸态，检查是否达到消抖时间
                if ((now - touch_debounce_timer) >= TOUCH_DEBOUNCE_MS) {
                    // ? 确认触摸结束
                    g_isTouching = 0;
                    g_touchEvent = 1;
                    g_touchProcessed = 0;
                    g_touchEndTime = now;
                    if (g_touchEndCallback != NULL) {
                        g_touchEndCallback();
                    }
                    touch_fsm_state = TOUCH_IDLE;
                }
            } else {
                // 释放消抖期间又变成高电平，回到激活态（抖动）
                touch_fsm_state = TOUCH_ACTIVE;
            }
            break;
    }

    // 处理事件标志（与原代码保持兼容）
    if (g_touchEvent && !g_touchProcessed) {
        g_touchEvent = 0;
        g_touchProcessed = 1;
    }

    // （可选）保留原代码中“持续触摸时重复害羞动作”的逻辑
    if (g_isTouching && g_touchStartCallback == Default_Touch_Start_Handler) {
        static uint32_t last_shy_repeat = 0;
        if (g_shyActionRunning && Servo_IsSequenceCompleted()) {
            g_shyActionRunning = 0;
            last_shy_repeat = now;
        }
        if (!g_shyActionRunning && (now - last_shy_repeat) > SHY_ACTION_INTERVAL) {
            Start_Shy_Action();
        }
    }
}

// 获取触摸模块状态信息（用于调试）
void Touch_Get_Status(uint8_t* is_touching, uint32_t* time_since_touch_end, uint32_t* delay_time)
{
    if (is_touching != NULL) {
        *is_touching = g_isTouching;
    }
    
    if (time_since_touch_end != NULL && g_touchEndTime > 0) {
        *time_since_touch_end = GetTick() - g_touchEndTime;
    }
    
    if (delay_time != NULL) {
        *delay_time = g_touch_delay_time;
    }
}
