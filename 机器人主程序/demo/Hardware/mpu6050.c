/**
  ******************************************************************************
  * @file    mpu6050.c
  * @author  生活中的小黑
  * @version V 2.0.0
  * @date    2025年8月4日
  * @brief   MPU6050传感器驱动与运动检测模块源文件
  ******************************************************************************
  */

#include "mpu6050.h"
#include "si2c.h"
#include "delay.h"
#include <math.h>
#include <stddef.h>  // 用于NULL定义

/* 软件 I2C 句柄 */
static SI2C_TypeDef si2c;

/* 原始数据 */
static float ax, ay, az, gx, gy, gz, temp;

/* 角度结果 */
static float yaw, roll, pitch;

/* 互补滤波系数 */
#define COMP_FILTER_COEF  0.98f       /* 陀螺仪权重 */
#define DT                0.005f      /* 5 ms更新周期，与Proc()调用间隔一致 */

/* 运动检测相关变量 */
static MPUState mpu_state = MPU_STATE_NORMAL;
static uint32_t shake_start_time = 0;
static uint32_t stable_start_time = 0;
static uint32_t last_dizzy_trigger_time = 0;
static uint32_t dizzy_extend_start_time = 0;
static MPUActionCallback action_callback = NULL;
/* 忽略窗口：在此时间之前忽略 trigger_action 回调 */
static uint32_t action_ignore_until = 0;

/* 额外软件滤波与确认参数 */
#define GYRO_FILTER_ALPHA 0.6f
#define SHAKE_CONFIRM_COUNT 2
#define SHAKE_HYSTERESIS_FACTOR 0.85f

#define ACCEL_FILTER_ALPHA 0.6f
#define ACCEL_DELTA_THRESHOLD 0.15f    /* g，重力变化阈值 */
/* Y轴加速度阈值，默认0.8g（更保守），可以通过 MPU6050_SetAccelShakeThreshold 调整 */
static float accel_shake_threshold = 0.8f;

/* 低通滤波缓存 (deg/s) */
static float f_gx = 0, f_gy = 0, f_gz = 0;
static float f_ax = 0, f_ay = 0, f_az = 0;
static int shake_confirm_count = 0;
static uint8_t last_shake_state = 0;

/* 躺倒检测相关变量 */
static MPULieCallback lie_callback = NULL;
static MPULieCallback stand_callback = NULL;
static uint8_t lie_state_entered = 0;   // 避免重复触发回调

/* 躺倒检测阈值（俯仰角绝对值大于此值视为躺倒） */
#define LIE_DOWN_PITCH_THRESHOLD  50.0f   // 50度

/* 晃动检测阈值 - 可根据实际效果调整 */
static float shake_threshold = 9.0f;           // 晃动阈值(rad/s)，值越小越灵敏
static const uint32_t DIZZY_TRIGGER_INTERVAL = 1000;    // 头晕触发间隔(ms) - 默认为1秒
static const uint32_t DIZZY_EXTEND_DURATION = 2000;     // 停止晃动后延长头晕的时间(ms)
static const uint32_t VOMIT_RECOVERY_TIME = 1500;       // 呕吐恢复时间(ms)

/* 底层寄存器读写 */
static void    reg_write(uint8_t reg, uint8_t data);
static uint8_t reg_read(uint8_t reg);

/* 内部函数 */
static void angles_update(void);
static void motion_detection_update(void);
static float calculate_gyro_magnitude(void);
static void trigger_action(FaceType face_type);
static uint8_t is_shaking(void);

/* ---------------- 公开接口 ---------------- */

/**
  * @brief  MPU6050初始化
  */
void App_MPU6050_Init(void)
{
    /* 1. 配置I2C引脚 */
    si2c.SCL_GPIOx  = GPIOB;  si2c.SCL_GPIO_Pin = GPIO_Pin_8;
    si2c.SDA_GPIOx  = GPIOB;  si2c.SDA_GPIO_Pin = GPIO_Pin_9;
    My_SI2C_Init(&si2c);

    /* 2. 复位 + 时钟 */
    reg_write(0x6B, 0x80); Delay_ms(100);
    reg_write(0x6B, 0x01);      // PLL 时钟
    reg_write(0x19, 0x00);      // 1 kHz 采样

    /* 3. 配置传感器 */
    reg_write(0x1B, 0x18);      // 陀螺仪 ±2000 °/s
    reg_write(0x1A, 0x02);      // 低通滤波 94 Hz
    reg_write(0x1C, 0x00);      // 加速度计 ±2 g
    reg_write(0x1D, 0x02);      // 加速度计滤波 92 Hz

    /* 4. 角度初始值 */
    yaw = roll = pitch = 0.0f;
    
    /* 5. 状态初始化 */
    mpu_state = MPU_STATE_NORMAL;
    shake_start_time = 0;
    stable_start_time = 0;
    last_dizzy_trigger_time = 0;
    dizzy_extend_start_time = 0;
    lie_state_entered = 0;
}

/**
  * @brief  更新MPU6050数据和状态
  */
void App_MPU6050_Update(void)
{
    int16_t ax_r = (int16_t)(reg_read(0x3B)<<8 | reg_read(0x3C));
    int16_t ay_r = (int16_t)(reg_read(0x3D)<<8 | reg_read(0x3E));
    int16_t az_r = (int16_t)(reg_read(0x3F)<<8 | reg_read(0x40));
    int16_t t_r  = (int16_t)(reg_read(0x41)<<8 | reg_read(0x42));
    int16_t gx_r = (int16_t)(reg_read(0x43)<<8 | reg_read(0x44));
    int16_t gy_r = (int16_t)(reg_read(0x45)<<8 | reg_read(0x46));
    int16_t gz_r = (int16_t)(reg_read(0x47)<<8 | reg_read(0x48));

    /* 单位转换 */
    ax = ax_r * 0.00006103515625f;   // g
    ay = ay_r * 0.00006103515625f;
    az = az_r * 0.00006103515625f;
    temp = t_r * 0.00294117647059f + 36.53f;
    gx = gx_r * 0.06097560975610f;   // °/s
    gy = gy_r * 0.06097560975610f;
    gz = gz_r * 0.06097560975610f;

    /* 应用 IIR 低通滤波到角速度（deg/s）以降低舵机引入的短时脉冲 */
    f_gx = GYRO_FILTER_ALPHA * f_gx + (1.0f - GYRO_FILTER_ALPHA) * gx;
    f_gy = GYRO_FILTER_ALPHA * f_gy + (1.0f - GYRO_FILTER_ALPHA) * gy;
    f_gz = GYRO_FILTER_ALPHA * f_gz + (1.0f - GYRO_FILTER_ALPHA) * gz;

    /* 应用 IIR 低通滤波到加速度（g）以抵抗瞬态震动 */
    f_ax = ACCEL_FILTER_ALPHA * f_ax + (1.0f - ACCEL_FILTER_ALPHA) * ax;
    f_ay = ACCEL_FILTER_ALPHA * f_ay + (1.0f - ACCEL_FILTER_ALPHA) * ay;
    f_az = ACCEL_FILTER_ALPHA * f_az + (1.0f - ACCEL_FILTER_ALPHA) * az;

    /* 更新角度 */
    angles_update();
    
    /* 运动检测 */
    motion_detection_update();
}

/**
  * @brief  注册动作回调函数
  * @param  callback: 回调函数指针
  */
void MPU6050_RegisterActionCallback(MPUActionCallback callback)
{
    action_callback = callback;
}

/**
  * @brief  注册躺倒回调函数
  * @param  callback: 回调函数指针
  */
void MPU6050_RegisterLieCallback(MPULieCallback callback)
{
    lie_callback = callback;
}

/**
  * @brief  注册站立恢复回调函数
  * @param  callback: 回调函数指针
  */
void MPU6050_RegisterStandCallback(MPULieCallback callback)
{
    stand_callback = callback;
}

/**
  * @brief  强制退出躺倒状态（用于触摸唤醒后外部恢复）
  */
void MPU6050_ForceExitLieState(void)
{
    if (mpu_state == MPU_STATE_LYING_DOWN) {
        mpu_state = MPU_STATE_NORMAL;
        if (stand_callback != NULL && lie_state_entered) {
            stand_callback();
            lie_state_entered = 0;
        }
    }
}

/**
  * @brief  设置晃动检测灵敏度
  * @param  sensitivity: 灵敏度阈值，值越小越灵敏
  */
void MPU6050_SetShakeSensitivity(float sensitivity)
{
    if (sensitivity > 0.1f && sensitivity < 10.0f) {
        shake_threshold = sensitivity;
    }
}

/**
  * @brief  设置基于Y轴加速度的晃动判定阈值（单位 g）
  * @param thresh 加速度阈值（例如 0.5 表示 0.5g）
  */
void MPU6050_SetAccelShakeThreshold(float thresh)
{
    if (thresh > 0.05f && thresh < 5.0f) {
        accel_shake_threshold = thresh;
    }
}

/**
  * @brief  获取当前MPU状态
  * @retval 当前状态
  */
MPUState MPU6050_GetCurrentState(void)
{
    return mpu_state;
}

/**
  * @brief  判断是否允许执行其他动作
  * @retval 1:允许, 0:不允许
  */
uint8_t MPU6050_IsAllowOtherActions(void)
{
    // 只有在正常状态下才允许其他动作
    return (mpu_state == MPU_STATE_NORMAL) ? 1 : 0;
}

/**
  * @brief  通知呕吐表情播放完成
  */
void MPU6050_NotifyVomitComplete(void)
{
    if (mpu_state == MPU_STATE_VOMIT_PLAYING) {
        mpu_state = MPU_STATE_RECOVERING;
        stable_start_time = GetTick(); // 开始恢复计时
    }
}

/**
  * @brief  忽略一段时间内MPU发起的动作回调，避免舵机动作自触发
  * @param  ms  忽略时间（毫秒）
  */
void MPU6050_IgnoreActionsFor(uint32_t ms)
{
    action_ignore_until = GetTick() + ms;
}

/* ---------- 内部静态函数 ---------- */

/**
  * @brief  运动检测状态机更新（包含躺倒检测）
  */
static void motion_detection_update(void)
{
    static uint32_t last_update_time = 0;
    uint32_t current_time = GetTick();
    
    // 控制检测频率，每100ms进行一次状态更新
    if (current_time - last_update_time < 100) {
        return;
    }
    last_update_time = current_time;
    
    // 在忽略窗口期间完全跳过检测，避免舵机动作导致的状态迁移
    if (current_time < action_ignore_until) {
        #ifdef DEBUG_ENABLE
        printf("MPU: in ignore window until %lu, skip check\n", (unsigned long)action_ignore_until);
        #endif
        last_update_time = current_time;
        return;
    }

    uint8_t shaking = is_shaking();
    float pitch_abs = (pitch > 0) ? pitch : -pitch;   // 俯仰角绝对值
    uint8_t is_lying = (pitch_abs > LIE_DOWN_PITCH_THRESHOLD) ? 1 : 0;

    switch (mpu_state) {
        case MPU_STATE_NORMAL:
            if (is_lying) {
                // 进入躺倒状态
                mpu_state = MPU_STATE_LYING_DOWN;
                if (lie_callback != NULL && !lie_state_entered) {
                    lie_callback();
                    lie_state_entered = 1;
                }
            } else if (shaking) {
                // 进入晃动状态
                mpu_state = MPU_STATE_SHAKING;
                shake_start_time = current_time;
                last_dizzy_trigger_time = current_time;
                if (current_time >= action_ignore_until) {
                    trigger_action(FACE_DIZZY);
                }
            }
            break;
            
        case MPU_STATE_SHAKING:
            if (is_lying) {
                // 躺倒优先级高于晃动
                mpu_state = MPU_STATE_LYING_DOWN;
                if (lie_callback != NULL && !lie_state_entered) {
                    lie_callback();
                    lie_state_entered = 1;
                }
            } else if (shaking) {
                // 持续晃动中，定期触发头晕表情
                if (current_time - last_dizzy_trigger_time >= DIZZY_TRIGGER_INTERVAL) {
                    if (current_time >= action_ignore_until) {
                        trigger_action(FACE_DIZZY);
                    }
                    last_dizzy_trigger_time = current_time;
                }
            } else {
                // 停止晃动，进入头晕延长状态
                mpu_state = MPU_STATE_DIZZY_EXTEND;
                dizzy_extend_start_time = current_time;
                if (current_time >= action_ignore_until) {
                    trigger_action(FACE_DIZZY);
                }
            }
            break;
            
        case MPU_STATE_DIZZY_EXTEND:
            if (is_lying) {
                mpu_state = MPU_STATE_LYING_DOWN;
                if (lie_callback != NULL && !lie_state_entered) {
                    lie_callback();
                    lie_state_entered = 1;
                }
            } else if (current_time - dizzy_extend_start_time >= DIZZY_EXTEND_DURATION) {
                // 3秒后触发呕吐表情
                mpu_state = MPU_STATE_VOMIT_PLAYING;
                if (current_time >= action_ignore_until) {
                    trigger_action(FACE_VOMIT);
                }
            } else if (shaking) {
                // 延长状态期间再次晃动，回到晃动状态
                mpu_state = MPU_STATE_SHAKING;
                last_dizzy_trigger_time = current_time;
            }
            break;
            
        case MPU_STATE_VOMIT_PLAYING:
            if (is_lying) {
                mpu_state = MPU_STATE_LYING_DOWN;
                if (lie_callback != NULL && !lie_state_entered) {
                    lie_callback();
                    lie_state_entered = 1;
                }
            }
            // 呕吐表情播放中，等待外部通知结束
            break;
            
        case MPU_STATE_RECOVERING:
            if (is_lying) {
                mpu_state = MPU_STATE_LYING_DOWN;
                if (lie_callback != NULL && !lie_state_entered) {
                    lie_callback();
                    lie_state_entered = 1;
                }
            } else if (current_time - stable_start_time >= VOMIT_RECOVERY_TIME) {
                mpu_state = MPU_STATE_NORMAL;
            }
            break;

        case MPU_STATE_LYING_DOWN:
            if (!is_lying) {
                // 站立恢复
                mpu_state = MPU_STATE_NORMAL;
                if (stand_callback != NULL && lie_state_entered) {
                    stand_callback();
                    lie_state_entered = 0;
                }
            }
            break;
    }
}

/**
  * @brief  判断是否在晃动状态（基于Y轴加速度）
  * @retval 1:晃动中, 0:稳定
  */
static uint8_t is_shaking(void)
{
    /* 使用滤波后的Y轴加速度作为摇晃判定依据（单位 g） */
    float ay_val = f_ay;
    float abs_ay = fabsf(ay_val);

    float threshold_enter = accel_shake_threshold;
    float threshold_exit = accel_shake_threshold * SHAKE_HYSTERESIS_FACTOR;

    #ifdef DEBUG_ENABLE
    printf("MPU: f_ay=%.3fg abs_ay=%.3fg thr_enter=%.3f thr_exit=%.3f count=%d\n", f_ay, abs_ay, threshold_enter, threshold_exit, shake_confirm_count);
    #endif

    if (last_shake_state) {
        /* 若已处于抖动状态，等待低于退出阈值且稳定才能回到非抖动 */
        if (abs_ay < threshold_exit) {
            shake_confirm_count = 0;
            last_shake_state = 0;
            return 0;
        }
        return 1;
    }

    /* 非抖动状态：要求连续多次样本超过阈值才确认抖动 */
    if (abs_ay > threshold_enter) {
        shake_confirm_count++;
        if (shake_confirm_count >= SHAKE_CONFIRM_COUNT) {
            shake_confirm_count = 0;
            last_shake_state = 1;
            return 1;
        }
    } else {
        shake_confirm_count = 0;
    }
    return 0;
}

/**
  * @brief  计算陀螺仪幅值（暂未使用，保留）
  * @retval 陀螺仪幅值(rad/s)
  */
static float calculate_gyro_magnitude(void)
{
    /* 使用滤波后的角速度 (deg/s) 计算幅值，再转换到 rad/s 用于阈值比较 */
    float gx_deg = f_gx;
    float gy_deg = f_gy;
    float gz_deg = f_gz;

    float gx_rad = gx_deg * 0.0174533f;
    float gy_rad = gy_deg * 0.0174533f;
    float gz_rad = gz_deg * 0.0174533f;

    return sqrtf(gx_rad * gx_rad + gy_rad * gy_rad + gz_rad * gz_rad);
}

/**
  * @brief  触发动作回调
  * @param  face_type: 要触发的表情类型
  */
static void trigger_action(FaceType face_type)
{
    if (action_callback != NULL) {
        action_callback(face_type);
    }
}

/* ---------- 角度计算函数 ---------- */

/**
  * @brief  互补滤波计算角度
  */
static void angles_update(void)
{
    /* 加速度计角度 */
    float roll_a  = atan2f(ay, az) * 57.2957795131f;   // rad -> 度
    float pitch_a = atan2f(-ax, sqrtf(ay*ay + az*az)) * 57.2957795131f;

    /* 首次使用加速度计角度初始化 */
    static uint8_t first = 1;
    if (first) {
        first = 0;
        roll  = roll_a;
        pitch = pitch_a;
        yaw   = 0.0f;
        return;
    }

    /* 互补滤波 */
    roll  = COMP_FILTER_COEF * (roll  + gy * DT) + (1.0f - COMP_FILTER_COEF) * roll_a;
    pitch = COMP_FILTER_COEF * (pitch + gx * DT) + (1.0f - COMP_FILTER_COEF) * pitch_a;
    yaw   = yaw + gz * DT;      /* 积分，无参考 */

    /* 限制到 ±180° */
    if (yaw >  180.0f) yaw -= 360.0f;
    if (yaw < -180.0f) yaw += 360.0f;
    if (roll >  180.0f) roll -= 360.0f;
    if (roll < -180.0f) roll += 360.0f;
    if (pitch >  180.0f) pitch -= 360.0f;
    if (pitch < -180.0f) pitch += 360.0f;
}

/* ---------- 原始数据 + 角度获取 ---------- */
float App_MPU6050_GetAccelX(void) { return ax; }
float App_MPU6050_GetAccelY(void) { return ay; }
float App_MPU6050_GetAccelZ(void) { return az; }

float App_MPU6050_GetGyroX(void)  { return gx; }
float App_MPU6050_GetGyroY(void)  { return gy; }
float App_MPU6050_GetGyroZ(void)  { return gz; }

float App_MPU6050_GetTemperature(void) { return temp; }

float App_MPU6050_GetYaw(void)   { return yaw; }
float App_MPU6050_GetRoll(void)  { return roll; }
float App_MPU6050_GetPitch(void) { return pitch; }

/* ---------- 底层 I2C 封装 ---------- */
static void reg_write(uint8_t reg, uint8_t data)
{
    My_SI2C_RegWriteBytes(&si2c, 0xD0, reg, &data, 1);
}

static uint8_t reg_read(uint8_t reg)
{
    uint8_t val;
    My_SI2C_RegReadBytes(&si2c, 0xD0, reg, &val, 1);
    return val;
}
