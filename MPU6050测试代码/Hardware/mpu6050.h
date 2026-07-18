#ifndef __MPU6050_H
#define __MPU6050_H

#include "stm32f10x.h"
#include <stddef.h>   // 提供 NULL 定义

// 定义软件I2C结构体
typedef struct {
    GPIO_TypeDef *SCL_GPIOx;
    uint16_t SCL_GPIO_Pin;
    GPIO_TypeDef *SDA_GPIOx;
    uint16_t SDA_GPIO_Pin;
} SI2C_TypeDef;

// 晃动检测回调函数指针类型
typedef void (*ShakeDetectedCallback)(void);

// 默认I2C引脚配置 (PB6/PB7)
#define MPU6050_DEFAULT_I2C_PINS { \
    .SCL_GPIOx = GPIOB, .SCL_GPIO_Pin = GPIO_Pin_8, \
    .SDA_GPIOx = GPIOB, .SDA_GPIO_Pin = GPIO_Pin_9  \
}

// MPU6050数据结构体
typedef struct {
    float Accel_X;  // X轴加速度 (g)
    float Accel_Y;  // Y轴加速度 (g)
    float Accel_Z;  // Z轴加速度 (g)
    float Temp;     // 温度 (°C)
    float Gyro_X;   // X轴角速度 (°/s)
    float Gyro_Y;   // Y轴角速度 (°/s)
    float Gyro_Z;   // Z轴角速度 (°/s)
    float Angle_X;  // X轴角度（俯仰角 Pitch）(°)
    float Angle_Y;  // Y轴角度（横滚角 Roll）(°)
} MPU6050_Data;

// 函数声明
void MPU6050_Init(SI2C_TypeDef *si2c);
void MPU6050_ReadData(MPU6050_Data *data);
uint8_t MPU6050_ReadReg(SI2C_TypeDef *si2c, uint8_t reg);
void MPU6050_RegisterShakeCallback(ShakeDetectedCallback callback);
void MPU6050_SetShakeThreshold(float threshold);

#endif
