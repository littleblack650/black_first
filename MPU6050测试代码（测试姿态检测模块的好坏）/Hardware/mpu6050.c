#include "mpu6050.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include <math.h>
#include "Delay.h"

// 寄存器地址
#define SMPLRT_DIV   0x19
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define TEMP_OUT_H   0x41
#define GYRO_XOUT_H  0x43
#define WHO_AM_I     0x75

#define MPU6050_ADDR (0x68 << 1)  // 0xD0写，0xD1读

// 定义PI若不存在
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 全局I2C句柄
static SI2C_TypeDef *g_si2c = NULL;

// 软件I2C底层函数 ------------------------------------------------
static void SI2C_Init(SI2C_TypeDef *si2c) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    if(si2c->SCL_GPIOx == GPIOA) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    else if(si2c->SCL_GPIOx == GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    else if(si2c->SCL_GPIOx == GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    if(si2c->SDA_GPIOx == GPIOA) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    else if(si2c->SDA_GPIOx == GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    else if(si2c->SDA_GPIOx == GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // 初始化SCL和SDA为开漏输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Pin = si2c->SCL_GPIO_Pin;
    GPIO_Init(si2c->SCL_GPIOx, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = si2c->SDA_GPIO_Pin;
    GPIO_Init(si2c->SDA_GPIOx, &GPIO_InitStructure);
    
    GPIO_SetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    GPIO_SetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
}

static void SI2C_Start(SI2C_TypeDef *si2c) {
    GPIO_SetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
    GPIO_SetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(5);
    GPIO_ResetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
    Delay_us(5);
    GPIO_ResetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(5);
}

static void SI2C_Stop(SI2C_TypeDef *si2c) {
    GPIO_ResetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    GPIO_ResetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
    Delay_us(5);
    GPIO_SetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(5);
    GPIO_SetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
    Delay_us(5);
}

static uint8_t SI2C_WaitAck(SI2C_TypeDef *si2c) {
    uint8_t ack = 0;
    GPIO_SetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
    Delay_us(2);
    GPIO_SetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(5);
    if (GPIO_ReadInputDataBit(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin) == Bit_RESET) ack = 1;
    GPIO_ResetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(2);
    return ack;
}

static void SI2C_Ack(SI2C_TypeDef *si2c) {
    GPIO_ResetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
    Delay_us(2);
    GPIO_SetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(5);
    GPIO_ResetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(2);
    GPIO_SetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
}

static void SI2C_NAck(SI2C_TypeDef *si2c) {
    GPIO_SetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
    Delay_us(2);
    GPIO_SetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(5);
    GPIO_ResetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
    Delay_us(2);
}

static void SI2C_SendByte(SI2C_TypeDef *si2c, uint8_t byte) {
    for (int8_t i = 7; i >= 0; i--) {
        GPIO_ResetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
        if (byte & (1 << i)) GPIO_SetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
        else GPIO_ResetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
        Delay_us(5);
        GPIO_SetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
        Delay_us(5);
        GPIO_ResetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
        Delay_us(2);
    }
}

static uint8_t SI2C_ReadByte(SI2C_TypeDef *si2c, uint8_t ack) {
    uint8_t byte = 0;
    GPIO_SetBits(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin);
    for (int8_t i = 7; i >= 0; i--) {
        GPIO_SetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
        Delay_us(5);
        if (GPIO_ReadInputDataBit(si2c->SDA_GPIOx, si2c->SDA_GPIO_Pin) == Bit_SET)
            byte |= (1 << i);
        GPIO_ResetBits(si2c->SCL_GPIOx, si2c->SCL_GPIO_Pin);
        Delay_us(5);
    }
    if (ack) SI2C_Ack(si2c);
    else SI2C_NAck(si2c);
    return byte;
}

static void MPU6050_WriteReg(SI2C_TypeDef *si2c, uint8_t reg, uint8_t data) {
    SI2C_Start(si2c);
    SI2C_SendByte(si2c, MPU6050_ADDR);
    SI2C_WaitAck(si2c);
    SI2C_SendByte(si2c, reg);
    SI2C_WaitAck(si2c);
    SI2C_SendByte(si2c, data);
    SI2C_WaitAck(si2c);
    SI2C_Stop(si2c);
    Delay_ms(1);
}

uint8_t MPU6050_ReadReg(SI2C_TypeDef *si2c, uint8_t reg) {
    uint8_t data = 0;
    SI2C_Start(si2c);
    SI2C_SendByte(si2c, MPU6050_ADDR);
    SI2C_WaitAck(si2c);
    SI2C_SendByte(si2c, reg);
    SI2C_WaitAck(si2c);
    SI2C_Start(si2c);
    SI2C_SendByte(si2c, MPU6050_ADDR | 0x01);
    SI2C_WaitAck(si2c);
    data = SI2C_ReadByte(si2c, 0);
    SI2C_Stop(si2c);
    return data;
}

static void MPU6050_ReadMulti(SI2C_TypeDef *si2c, uint8_t reg, uint8_t *buf, uint8_t len) {
    SI2C_Start(si2c);
    SI2C_SendByte(si2c, MPU6050_ADDR);
    SI2C_WaitAck(si2c);
    SI2C_SendByte(si2c, reg);
    SI2C_WaitAck(si2c);
    SI2C_Start(si2c);
    SI2C_SendByte(si2c, MPU6050_ADDR | 0x01);
    SI2C_WaitAck(si2c);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = SI2C_ReadByte(si2c, (i == len - 1) ? 0 : 1);
    }
    SI2C_Stop(si2c);
}

// MPU6050初始化
void MPU6050_Init(SI2C_TypeDef *si2c) {
    g_si2c = si2c;
    SI2C_Init(si2c);
    MPU6050_WriteReg(si2c, PWR_MGMT_1, 0x80);
    Delay_ms(100);
    MPU6050_WriteReg(si2c, PWR_MGMT_1, 0x00);
    MPU6050_WriteReg(si2c, SMPLRT_DIV, 0x07);
    MPU6050_WriteReg(si2c, CONFIG, 0x06);
    MPU6050_WriteReg(si2c, GYRO_CONFIG, 0x18);
    MPU6050_WriteReg(si2c, ACCEL_CONFIG, 0x18);
}

// 读取数据并计算角度
void MPU6050_ReadData(MPU6050_Data *data) {
    uint8_t buf[14];
    int16_t raw_accel[3], raw_temp, raw_gyro[3];
    
    MPU6050_ReadMulti(g_si2c, ACCEL_XOUT_H, buf, 14);
    
    raw_accel[0] = (int16_t)((buf[0] << 8) | buf[1]);
    raw_accel[1] = (int16_t)((buf[2] << 8) | buf[3]);
    raw_accel[2] = (int16_t)((buf[4] << 8) | buf[5]);
    raw_temp     = (int16_t)((buf[6] << 8) | buf[7]);
    raw_gyro[0]  = (int16_t)((buf[8] << 8) | buf[9]);
    raw_gyro[1]  = (int16_t)((buf[10] << 8) | buf[11]);
    raw_gyro[2]  = (int16_t)((buf[12] << 8) | buf[13]);
    
    // 量程转换 (±16g -> 16384 LSB/g, ±2000dps -> 16.4 LSB/°/s)
    data->Accel_X = (float)raw_accel[0] / 16384.0f;
    data->Accel_Y = (float)raw_accel[1] / 16384.0f;
    data->Accel_Z = (float)raw_accel[2] / 16384.0f;
    data->Temp    = (float)raw_temp / 340.0f + 36.53f;
    data->Gyro_X  = (float)raw_gyro[0] / 16.4f;
    data->Gyro_Y  = (float)raw_gyro[1] / 16.4f;
    data->Gyro_Z  = (float)raw_gyro[2] / 16.4f;
    
    // 根据加速度计算俯仰角(Pitch)和横滚角(Roll)
    data->Angle_X = atan2f(data->Accel_Y, data->Accel_Z) * 180.0f / M_PI;
    data->Angle_Y = atan2f(-data->Accel_X, sqrtf(data->Accel_Y*data->Accel_Y + data->Accel_Z*data->Accel_Z)) * 180.0f / M_PI;
}

// 晃动检测（可留空，不影响主要功能）
void MPU6050_RegisterShakeCallback(ShakeDetectedCallback callback) {
    // 无需实现
}

void MPU6050_SetShakeThreshold(float threshold) {
    // 无需实现
}
