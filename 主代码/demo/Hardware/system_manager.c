/**
  ******************************************************************************
  * @file    system_manager.c
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   系统管理器 - 统一管理系统初始化和模块协调
  ******************************************************************************
  */

#include "system_manager.h"
#include "system_config.h"
#include "task_scheduler.h"
#include "delay.h"
#include "oled.h"
#include "W25Q64.h"
#include "face.h"
#include "pwm.h"
#include "touch.h"
#include "mpu6050.h"
#include "usart.h"
#include "random.h"
#include "battery.h"

/* 系统状态 */
static SystemState system_state = SYSTEM_STATE_INIT;
static uint8_t wake_completed = 0;

/* MPU6050相关变量 */
static uint8_t mpu_vomit_playing = 0;
static uint8_t mpu_dizzy_playing = 0;
static uint32_t last_dizzy_check_time = 0;

/* 电池相关变量 */
static uint8_t battery_low_handled = 0;
static uint32_t last_battery_warning_time = 0;
static const uint32_t BATTERY_WARNING_INTERVAL = 30000; // 低电量警告间隔30秒

/* 躺倒挣扎相关变量 */
static uint8_t lying_struggle_active = 0;      // 躺倒挣扎动作是否正在循环
static uint32_t last_struggle_trigger = 0;
static const uint32_t STRUGGLE_REPEAT_INTERVAL = 2000;   // 挣扎动作重复间隔(ms)

/* 触摸/语音睡眠定格标志 */
static uint8_t sleep_static_mode = 0;
static uint8_t sleep_triggered = 0;             // 触发睡眠等待定格

/* 私有函数声明 */
static void System_RegisterTasks(void);
static void MPU6050_ActionHandler(FaceType face_type);
static void VoiceCommandHandler(VoiceCommand cmd);
static void ExpressionWithServo(FaceType currentFace);
static void Battery_HandleLowPower(void);
static void SystemManager_OnLieDown(void);
static void SystemManager_OnStandUp(void);

/* 任务函数声明 */
static void Task_MPU6050_Update(void);
static void Task_Touch_Process(void);
static void Task_Voice_Process(void);
static void Task_Face_Update(void);
static void Task_Servo_Update(void);
static void Task_RandomFace_Update(void);
static void Task_System_Monitor(void);
static void Task_Battery_Update(void);

/**
  * @brief  系统初始化
  */
void SystemManager_Init(void)
{
    system_state = SYSTEM_STATE_INIT;
    
    // 1. 初始化基础模块
    Delay_Init();
    OLED_Init();
    OLED_Clear();
    W25Q64_Init();
    
    // 2. 初始化运动模块
    Face_Init();
    App_PWM_Init();
    Action_Init();
    Touch_Init();
    Battery_Init();  // 初始化电池检测
    
    // 3. 初始化传感器
    App_MPU6050_Init();
    MPU6050_RegisterActionCallback(MPU6050_ActionHandler);
    MPU6050_RegisterLieCallback(SystemManager_OnLieDown);
    MPU6050_RegisterStandCallback(SystemManager_OnStandUp);
    
    // 4. 初始化通信
    USART1_Config();
    USART_RegisterVoiceCallback(VoiceCommandHandler);
    
    // 5. 初始化任务调度器
    TaskScheduler_Init();
    System_RegisterTasks();

    system_state = SYSTEM_STATE_READY;
}

/**
  * @brief  启动系统
  */
void SystemManager_Start(void)
{
    if (system_state != SYSTEM_STATE_READY) {
        return;
    }
    
    // 唤醒机器人
    system_state = SYSTEM_STATE_WAKING;
    Face_RequestExpression(FACE_WAKE, FACE_PRIORITY_HIGH, 0);
    
    // 等待唤醒完成
    while (!Face_Update()) {
        Servo_Update_NonBlocking();
        Delay_ms(10);
    }
    
    wake_completed = 1;
    system_state = SYSTEM_STATE_RUNNING;
    
    // 初始化随机表情系统
    RandomFace_Init(NULL);
    
    // 启动任务调度器
    TaskScheduler_Start();
}

/**
  * @brief  系统主循环
  */
void SystemManager_Run(void)
{
    if (system_state != SYSTEM_STATE_RUNNING) {
        return;
    }
    
    // 运行任务调度器
    TaskScheduler_Run();
    
    // 系统特殊情形处理
    SystemManager_UpdateSpecialCases();
}

/**
  * @brief  特殊情形的处理
  */
void SystemManager_UpdateSpecialCases(void)
{
    uint32_t current_time = GetTick();
    FaceType currentFace = Face_GetCurrentExpression();
    uint8_t face_completed = Face_Update();

    // 如果之前通过回调触发了 dizzy，但表情已经结束，清理标志，避免重复触发
    if (mpu_dizzy_playing && currentFace != FACE_DIZZY) {
        mpu_dizzy_playing = 0;
    }
    
    // 1. 如果处于低电量状态，优先处理低电量提醒
    if (Battery_IsLow()) {
        // 确保低电量表情显示
        if (currentFace != FACE_RESPOND3 || face_completed) {
            Face_RequestExpression(FACE_RESPOND3, FACE_PRIORITY_HIGHEST, 1);
        }
        return; // 低电量状态下不处理其他动作
    }
    
    // 2. 处理呕吐表情完成通知
    if (mpu_vomit_playing && face_completed) {
        mpu_vomit_playing = 0;
        MPU6050_NotifyVomitComplete();
    }
    
    // 3. 在晃动状态确保头晕表情持续
    if ((MPU6050_GetCurrentState() == MPU_STATE_SHAKING || 
         MPU6050_GetCurrentState() == MPU_STATE_DIZZY_EXTEND) &&
        (current_time - last_dizzy_check_time > 500)) {
        
        last_dizzy_check_time = current_time;
        
        // 如果已经由回调处理（mpu_dizzy_playing），则跳过轮询触发
        if ((currentFace != FACE_DIZZY || face_completed) && !mpu_dizzy_playing) {
            Face_RequestExpression(FACE_DIZZY, FACE_PRIORITY_HIGHEST, 0);
            mpu_dizzy_playing = 1;
            
            if (!Servo_IsSequenceCompleted()) {
                Servo_StopSequence();
                Delay_ms(50);
            }
            
            Head_Shake_NonBlocking();
        }
    }
    
    // 4. 表情与舵机联动（仅在唤醒时且非其他指令触发的表情）
    if (wake_completed && Is_Allow_Other_Actions() && MPU6050_IsAllowOtherActions()) {
        // 排除特殊指令触发的表情响应
        if (face_completed == 0 && 
            currentFace != FACE_DIZZY && 
            currentFace != FACE_VOMIT &&
            currentFace != FACE_RESPOND1 &&
            currentFace != FACE_BT &&
            currentFace != FACE_RESPOND3) {
            ExpressionWithServo(currentFace);
        }
    }

    // 躺倒挣扎动作循环
    if (lying_struggle_active && MPU6050_GetCurrentState() == MPU_STATE_LYING_DOWN) {
        if (!sleep_static_mode && Servo_IsSequenceCompleted() && 
            (current_time - last_struggle_trigger >= STRUGGLE_REPEAT_INTERVAL)) {
            Action_Struggle_NonBlocking();
            last_struggle_trigger = current_time;
            if (Face_GetCurrentExpression() != FACE_STRUGGLE) {
                Face_RequestExpression(FACE_STRUGGLE, FACE_PRIORITY_HIGHEST, 1);
            }
        }
    }

    // 躺倒状态下触摸触发睡眠（原有逻辑）
    if (!sleep_static_mode && lying_struggle_active && 
        MPU6050_GetCurrentState() == MPU_STATE_LYING_DOWN && Is_Touching()) {
        sleep_static_mode = 1;
        sleep_triggered = 1;
        lying_struggle_active = 0;
        Servo_StopSequence();
        Face_RequestExpression(FACE_SLEEP, FACE_PRIORITY_HIGHEST, 1);
    }

    // 检测睡眠表情是否播放完成，然后进入静态帧（支持触摸和语音触发）
    if (sleep_triggered && Face_GetCurrentExpression() == FACE_NONE && !Face_IsStaticMode()) {
        // 定格到睡眠表情的最后一帧（地址 0x041000）
        Face_EnterStaticMode(0x041000);
        sleep_triggered = 0;
    }
}

/**
  * @brief  获取系统状态
  */
SystemState SystemManager_GetState(void)
{
    return system_state;
}

/**
  * @brief  系统紧急停止
  */
void SystemManager_EmergencyStop(void)
{
    TaskScheduler_Stop();
    
    // 停止所有动作
    Servo_StopSequence();
    
    // 清空表情模块
    Face_RequestExpression(FACE_NONE, FACE_PRIORITY_HIGHEST, 1);
    
    system_state = SYSTEM_STATE_STOPPED;
}

/**
  * @brief  系统恢复运行
  */
void SystemManager_Resume(void)
{
    if (system_state == SYSTEM_STATE_STOPPED) {
        system_state = SYSTEM_STATE_RUNNING;
        TaskScheduler_Start();
    }
}

/* ========== 私有函数实现 ========== */

/**
  * @brief  注册系统任务
  */
static void System_RegisterTasks(void)
{
    // 高优先级任务 - 传感器实时处理
    TaskScheduler_Register(Task_MPU6050_Update, 5, TASK_PRIORITY_HIGH, "MPU6050");
    
    // 中优先级任务 - 用户交互和系统监测
    TaskScheduler_Register(Task_Touch_Process, 10, TASK_PRIORITY_MEDIUM, "Touch");
    TaskScheduler_Register(Task_Voice_Process, 20, TASK_PRIORITY_MEDIUM, "Voice");
    TaskScheduler_Register(Task_Battery_Update, 1000, TASK_PRIORITY_MEDIUM, "Battery");
    
    // 低优先级任务 - 表情和动作
    TaskScheduler_Register(Task_Face_Update, 20, TASK_PRIORITY_LOW, "Face");
    TaskScheduler_Register(Task_Servo_Update, 10, TASK_PRIORITY_LOW, "Servo");
    TaskScheduler_Register(Task_RandomFace_Update, 100, TASK_PRIORITY_LOW, "RandomFace");
    
    // 系统监控任务
    TaskScheduler_Register(Task_System_Monitor, 1000, TASK_PRIORITY_LOW, "Monitor");
}

/**
  * @brief  MPU6050动作回调
  */
static void MPU6050_ActionHandler(FaceType face_type)
{
    // 低电量状态下不响应MPU6050动作
    if (Battery_IsLow()) {
        return;
    }
    
    if (!Servo_IsSequenceCompleted()) {
        Servo_StopSequence();
//        Delay_ms(50);         //为防止阻塞试验性注销该延迟
    }
    
    Face_RequestExpression(face_type, FACE_PRIORITY_HIGHEST, 0);
    
    switch(face_type) {
        case FACE_DIZZY:
            mpu_dizzy_playing = 1;
            Head_Shake_NonBlocking();
            break;
            
        case FACE_VOMIT:
            mpu_vomit_playing = 1;
            Head_Down_NonBlocking();
            Action_Struggle_NonBlocking();
            break;
            
        default:
            break;
    }
}

/**
  * @brief  判断是否允许响应语音指令（静态模式下只允许唤醒）
  */
static uint8_t IsVoiceCommandAllowed(void)
{
    // 获取当前状态
    FaceType currentFace = Face_GetCurrentExpression();
    MPUState mpuState = MPU6050_GetCurrentState();
    
    // 不允许响应语音指令的情况
    // 1. 电池低电量状态
    // 2. MPU6050正在播放呕吐动画
    // 3. MPU6050正在播放头晕动画
    // 4. 当前正在播放重要表情（呕吐、头晕、低电量表情）
    if (Battery_IsLow() ||
        mpuState == MPU_STATE_VOMIT_PLAYING ||
        mpuState == MPU_STATE_DIZZY_EXTEND ||
        currentFace == FACE_VOMIT ||
        currentFace == FACE_DIZZY ||
        currentFace == FACE_RESPOND3) {
        return 0;
    }
    
    return 1;
}

/**
  * @brief  语音指令处理回调（增加睡眠/唤醒处理）
  */
static void VoiceCommandHandler(VoiceCommand cmd)
{
    static VoiceCommand lastCmd = VOICE_CMD_NONE;
    static uint32_t lastSendTime = 0;
    uint32_t currentTime = GetTick();
    
    // 防止重复处理相同指令
    if (cmd == lastCmd && (currentTime - lastSendTime < 500)) {
        return;
    }
    
    // **新增：如果处于静态帧睡眠模式，只允许唤醒指令（VOICE_CMD_1F）**
    if (Face_IsStaticMode() && cmd != VOICE_CMD_1F) {
        #ifdef DEBUG_ENABLE
        printf("Voice ignored: in static sleep mode, only wakeup command allowed\n");
        #endif
        return;
    }
    
    // 检查是否允许响应语音指令（排除低电量等）
    if (!IsVoiceCommandAllowed()) {
        return;
    }
    
    lastCmd = cmd;
    lastSendTime = currentTime;
    
    // 如果当前有动作序列正在执行，先停止（保留高优先级表情）
    if (!Servo_IsSequenceCompleted()) {
        Servo_StopSequence();
//        Delay_ms(50);            //为防止阻塞试验性注销该延迟
    }
    
    // **新增：处理睡眠指令 VOICE_CMD_1E -> FACE_SLEEP**
    if (cmd == VOICE_CMD_1E) {
        // 停止挣扎等动作
        lying_struggle_active = 0;
        // 设置睡眠定格标志，等待表情播放完成后进入静态帧
        sleep_static_mode = 1;
        sleep_triggered = 1;
        // 停止当前舵机动作
        Servo_StopSequence();
        // 请求睡眠表情
        Face_RequestExpression(FACE_SLEEP, FACE_PRIORITY_HIGHEST, 1);
        #ifdef DEBUG_ENABLE
        printf("Voice sleep triggered\n");
        #endif
        return;
    }
    
    // **新增：处理唤醒指令 VOICE_CMD_1F -> FACE_WAKE**
    if (cmd == VOICE_CMD_1F) {
        // 退出静态帧模式（如果处于睡眠定格）
        if (Face_IsStaticMode()) {
            Face_ExitStaticMode();
        }
        // 清除睡眠相关标志
        sleep_static_mode = 0;
        sleep_triggered = 0;
        // 唤醒动作：播放唤醒表情
        Face_RequestExpression(FACE_WAKE, FACE_PRIORITY_HIGH, 0);
        // 执行一个简单的抬头动作
        Head_Up_NonBlocking();
        #ifdef DEBUG_ENABLE
        printf("Voice wakeup triggered\n");
        #endif
        return;
    }
    
    // 其他语音指令的正常处理
    FaceType face_type = VoiceCommand_To_FaceType(cmd);
    
    // 对 KEBI/KEBISING 提高优先级
    if (face_type == FACE_KEBI || face_type == FACE_KEBISING) {
        Face_RequestExpression(face_type, FACE_PRIORITY_HIGH, 0);
    } else {
        Face_RequestExpression(face_type, FACE_PRIORITY_MEDIUM, 0);
    }
    
    Face_Update();  // 刷新表情状态
    
    switch (face_type) {
        case FACE_RESPOND1:
        case FACE_BT:
        case FACE_RESPOND3:
            Head_Response_NonBlocking();
            break;
            
        case FACE_STRUGGLE:
            Action_Struggle_NonBlocking();
            break;
            
        case FACE_SMILE:
        case FACE_KEBI:
        case FACE_KEBISING:
        case FACE_BAINIAN:
            Action_Smile_NonBlocking();
            break;
            
        case FACE_ANGRY:
        case FACE_ANGRY1:
        case FACE_ANGRY2:
        case FACE_ANGRY3:
            Head_Shake_NonBlocking();
            break;
            
        case FACE_UP:
            Head_Up_NonBlocking();
            break;
            
        case FACE_DOWN:
            Head_Down_NonBlocking();
            break;
            
        case FACE_LEFT:
            Head_Left_NonBlocking();
            break;
            
        case FACE_RIGHT:
            Head_Right_NonBlocking();
            break;
            
        case FACE_LEFTUP:
            Head_LU_NonBlocking();
            break;
            
        case FACE_RIGHTUP:
            Head_RU_NonBlocking();
            break;
            
        case FACE_LEFTDOWN:
            Head_LD_NonBlocking();
            break;
            
        case FACE_RIGHTDOWN:
            Head_RD_NonBlocking();
            break;
            
        case FACE_CRY:
            Head_Down_NonBlocking();
            break;
            
        case FACE_SAD:
            Action_Sad_NonBlocking();
            break;
            
        case FACE_DOUBT:
            Head_Shake_NonBlocking();
            break;
            
        case FACE_EAT:
            Complex_Eat_NonBlocking();
            break;
            
        case FACE_VOMIT:
            Head_Down_NonBlocking();
            Action_Struggle_NonBlocking();
            break;
            
        case FACE_LOVE:
            Head_Around_NonBlocking();
            break;
        
        case FACE_DANCE:
            Complex_Dance_NonBlocking();
            break;
            
        case FACE_SLEEP:
        case FACE_WAKE:
        case FACE_BLINK:
            // 已单独处理或无动作
            break;
            
        default:
            Head_Response_NonBlocking();
            break;
    }
    
    #ifdef DEBUG_ENABLE
    printf("Voice command received: 0x%02X -> Face: %d\n", cmd, face_type);
    #endif
}

/**
  * @brief  表情与舵机联动
  */
static void ExpressionWithServo(FaceType currentFace)
{
    static FaceType lastFace = FACE_NONE;
    static uint32_t lastActionTime = 0;
    const uint32_t actionCooldown = 100;
    
    uint32_t currentTime = GetTick();
    
    if (currentFace != lastFace && 
        (currentTime - lastActionTime) > actionCooldown &&
        currentFace != FACE_DIZZY && 
        currentFace != FACE_VOMIT &&
        currentFace != FACE_RESPOND1 &&
        currentFace != FACE_BT &&
        currentFace != FACE_RESPOND3) {
        
        lastFace = currentFace;
        lastActionTime = currentTime;
        
        if (!Servo_IsSequenceCompleted()) {
            Servo_StopSequence();
//            Delay_ms(50);
        }
        
        switch(currentFace) {
            case FACE_SMILE: Action_Smile_NonBlocking(); break;
            case FACE_SAD: Action_Sad_NonBlocking(); break;
            case FACE_ANGRY: Head_Shake_NonBlocking(); break;
            case FACE_COMFORTABLE: Action_Comfortable_NonBlocking(); break;
            case FACE_EAT: Complex_Eat_NonBlocking(); break;
            case FACE_LEFT: Head_Left_NonBlocking(); break;
            case FACE_RIGHT: Head_Right_NonBlocking(); break;
            case FACE_UP: Head_Up_NonBlocking(); break;
            case FACE_DOWN: Head_Down_NonBlocking(); break;
            case FACE_LEFTUP: Head_LU_NonBlocking(); break;
            case FACE_RIGHTUP: Head_RU_NonBlocking(); break;
            case FACE_LEFTDOWN: Head_LD_NonBlocking(); break;
            case FACE_RIGHTDOWN: Head_RD_NonBlocking(); break;
            case FACE_DOUBT: Head_Shake_NonBlocking(); break;
            case FACE_LOVE: Head_Around_NonBlocking(); break;
            case FACE_DANCE: Complex_Dance_NonBlocking(); break;
            case FACE_STRUGGLE: Action_Struggle_NonBlocking(); break;
            case FACE_CRY: Head_Down_NonBlocking(); break;
            default: break;
        }
        MPU6050_IgnoreActionsFor(2500);
    }
}

/* 躺倒和站立回调 */
static void SystemManager_OnLieDown(void)
{
    if (!Servo_IsSequenceCompleted()) {
        Servo_StopSequence();
        Delay_ms(50);
    }
    Face_RequestExpression(FACE_STRUGGLE, FACE_PRIORITY_HIGHEST, 1);
    Action_Struggle_NonBlocking();
    lying_struggle_active = 1;
    last_struggle_trigger = GetTick();
    sleep_static_mode = 0;
    sleep_triggered = 0;
}

static void SystemManager_OnStandUp(void)
{
    lying_struggle_active = 0;
    if (Face_IsStaticMode()) {
        Face_ExitStaticMode();
    }
    Face_RequestExpression(FACE_NONE, FACE_PRIORITY_HIGHEST, 1);
    Servo_StopSequence();
    Action_Init();
    sleep_static_mode = 0;
    sleep_triggered = 0;
}

/* 电池监测任务 */
static void Task_Battery_Update(void)
{
    Battery_Update();
    Battery_HandleLowPower();
}

static void Battery_HandleLowPower(void)
{
    uint32_t current_time = GetTick();
    
    if (Battery_IsLow() && !battery_low_handled) {
        battery_low_handled = 1;
        last_battery_warning_time = current_time;
        RandomFace_SetEnable(0);
        if (!Servo_IsSequenceCompleted()) {
            Servo_StopSequence();
        }
        Face_RequestExpression(FACE_RESPOND3, FACE_PRIORITY_HIGHEST, 1);
        MPU6050_IgnoreActionsFor(2500);
        Head_Down_NonBlocking();
        #ifdef DEBUG_ENABLE
        printf("Low battery warning! Voltage: %.2fV\n", Battery_GetVoltage());
        #endif
    } else if (Battery_IsLow() && battery_low_handled) {
        if (current_time - last_battery_warning_time >= BATTERY_WARNING_INTERVAL) {
            last_battery_warning_time = current_time;
            if (Face_GetCurrentExpression() != FACE_RESPOND3) {
                Face_RequestExpression(FACE_RESPOND3, FACE_PRIORITY_HIGHEST, 1);
            }
            MPU6050_IgnoreActionsFor(2500);
            Head_Down_NonBlocking();
            #ifdef DEBUG_ENABLE
            printf("Low battery reminder! Voltage: %.2fV\n", Battery_GetVoltage());
            #endif
        }
    } else if (!Battery_IsLow() && battery_low_handled) {
        battery_low_handled = 0;
        Face_RequestExpression(FACE_NONE, FACE_PRIORITY_HIGHEST, 1);
        RandomFace_SetEnable(1);
        #ifdef DEBUG_ENABLE
        printf("Battery recovered! Voltage: %.2fV\n", Battery_GetVoltage());
        #endif
    }
}

/* 任务实现 */
static void Task_MPU6050_Update(void) { App_MPU6050_Update(); }
static void Task_Touch_Process(void) { Touch_Process(); }
static void Task_Voice_Process(void) { USART_ProcessVoiceData(); }
static void Task_Face_Update(void) { Face_Update(); }
static void Task_Servo_Update(void)
{
    Servo_Update_NonBlocking();
    Servo_UpdateSequence();
}
static void Task_RandomFace_Update(void)
{
    if (wake_completed && Is_Allow_Other_Actions() && MPU6050_IsAllowOtherActions()) {
        RandomFace_Update();
    }
}
static void Task_System_Monitor(void) { /* 预留 */ }
