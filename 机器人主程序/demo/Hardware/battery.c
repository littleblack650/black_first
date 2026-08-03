/**
  ******************************************************************************
  * @file    battery.c
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   电池电量检测模块 - 使用PA4引脚
  ******************************************************************************
  */

#include "battery.h"
#include "stm32f10x.h"
#include "delay.h"
#include "system_config.h"
#include "face.h"
#include "usart.h"

/* 电池状态变量 */
static BatteryState battery_state = BATTERY_STATE_NORMAL;
static float battery_voltage = 0.0f;
static uint32_t last_battery_check_time = 0;
static uint8_t low_battery_reported = 0;
static uint8_t charging_detected = 0;

/* 电压阈值配置 */
#define BATTERY_CHECK_INTERVAL   5000    // 电池检测间隔5秒
#define VOLTAGE_FILTER_COEF      0.1f    // 电压滤波系数
#define LOW_VOLTAGE_THRESHOLD    3.4f    // 低电量阈值(约1/3电量)
#define CHARGING_VOLTAGE         4.0f    // 充电检测电压阈值
#define VOLTAGE_NORMAL_THRESHOLD 3.6f    // 恢复正常电压阈值

/* ADC相关定义 - 修改为使用PA4 */
#define ADC_CHANNEL      ADC_Channel_4   // 使用PA4作为ADC输入 (ADC12_IN4)
#define ADC_SAMPLE_TIMES 10              // ADC采样次数

/* 私有函数声明 */
static void Battery_ADC_Init(void);
static uint16_t Battery_ADC_Read(void);
static float Battery_CalculateVoltage(uint16_t adc_value);
static void Battery_UpdateState(void);

/**
  * @brief  电池检测初始化
  */
void Battery_Init(void)
{
    // 初始化ADC
    Battery_ADC_Init();
    
    // 初始化状态变量
    battery_state = BATTERY_STATE_NORMAL;
    battery_voltage = 0.0f;
    last_battery_check_time = 0;
    low_battery_reported = 0;
    charging_detected = 0;
    
    // 首次读取电池电压
    Battery_UpdateVoltage();
}

/**
  * @brief  更新电池电压测量
  */
void Battery_UpdateVoltage(void)
{
    uint16_t adc_raw = Battery_ADC_Read();
    float new_voltage = Battery_CalculateVoltage(adc_raw);
    
    // 使用低通滤波平滑电压值
    if (battery_voltage == 0.0f) {
        battery_voltage = new_voltage;
    } else {
        battery_voltage = battery_voltage * (1.0f - VOLTAGE_FILTER_COEF) + 
                         new_voltage * VOLTAGE_FILTER_COEF;
    }
}

/**
  * @brief  电池检测更新任务
  */
void Battery_Update(void)
{
    uint32_t current_time = GetTick();
    
    // 定期检测电池状态
    if (current_time - last_battery_check_time >= BATTERY_CHECK_INTERVAL) {
        last_battery_check_time = current_time;
        
        // 更新电压测量
        Battery_UpdateVoltage();
        
        // 更新电池状态
        Battery_UpdateState();
    }
}

/**
  * @brief  获取当前电池电压
  */
float Battery_GetVoltage(void)
{
    return battery_voltage;
}

/**
  * @brief  获取电池状态
  */
BatteryState Battery_GetState(void)
{
    return battery_state;
}

/**
  * @brief  检查是否处于低电量状态
  */
uint8_t Battery_IsLow(void)
{
    return (battery_state == BATTERY_STATE_LOW);
}

/**
  * @brief  检查是否正在充电
  */
uint8_t Battery_IsCharging(void)
{
    return charging_detected;
}

/**
  * @brief  强制触发低电量警告（用于测试）
  */
void Battery_ForceLowWarning(void)
{
    battery_state = BATTERY_STATE_LOW;
    low_battery_reported = 0;  // 重置报告标志，确保触发警告
}

/**
  * @brief  强制恢复正常状态（用于测试）
  */
void Battery_ForceNormal(void)
{
    battery_state = BATTERY_STATE_NORMAL;
    low_battery_reported = 0;
}

/* ========== 私有函数实现 ========== */

/**
  * @brief  ADC初始化 - 修改为使用PA4
  */
static void Battery_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    
    // 1. 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    
    // 2. 配置PA4为模拟输入 (修改为PA4)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;  // PA4
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. 配置ADC
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // 4. 配置ADC通道 - 修改为ADC_Channel_4
    ADC_RegularChannelConfig(ADC1, ADC_CHANNEL, 1, ADC_SampleTime_55Cycles5);
    
    // 5. 使能ADC
    ADC_Cmd(ADC1, ENABLE);
    
    // 6. ADC校准
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

/**
  * @brief  ADC读取函数
  */
static uint16_t Battery_ADC_Read(void)
{
    uint32_t adc_sum = 0;
    
    // 多次采样取平均
    for (uint8_t i = 0; i < ADC_SAMPLE_TIMES; i++) {
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
        while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
        adc_sum += ADC_GetConversionValue(ADC1);
    }
    
    return (uint16_t)(adc_sum / ADC_SAMPLE_TIMES);
}

/**
  * @brief  计算实际电压值
  * @note   假设使用电阻分压，分压比为2:1，参考电压3.3V
  */
static float Battery_CalculateVoltage(uint16_t adc_value)
{
    // ADC值转换为电压 (0-3.3V)
    float adc_voltage = (adc_value * 3.3f) / 4095.0f;
    
    // 计算实际电池电压（假设分压电阻比例为2:1）
    // 实际电压 = ADC电压 * (R1+R2)/R2 = ADC电压 * 3
    float actual_voltage = adc_voltage * 3.0f;
    
    return actual_voltage;
}

/**
  * @brief  更新电池状态
  */
static void Battery_UpdateState(void)
{
    BatteryState old_state = battery_state;
    
    // 检测充电状态（电压突然升高）
    if (battery_voltage > CHARGING_VOLTAGE) {
        charging_detected = 1;
        battery_state = BATTERY_STATE_NORMAL;
        low_battery_reported = 0;  // 重置低电量报告标志
    } 
    // 检测低电量状态
    else if (battery_voltage < LOW_VOLTAGE_THRESHOLD) {
        charging_detected = 0;
        battery_state = BATTERY_STATE_LOW;
    }
    // 恢复正常状态
    else if (battery_voltage > VOLTAGE_NORMAL_THRESHOLD) {
        charging_detected = 0;
        battery_state = BATTERY_STATE_NORMAL;
        low_battery_reported = 0;  // 重置低电量报告标志
    }
    
    // 状态变化调试信息
    #ifdef DEBUG_ENABLE
    if (old_state != battery_state) {
        printf("Battery state changed: %d -> %d, Voltage: %.2fV\n", 
               old_state, battery_state, battery_voltage);
    }
    #endif
}
