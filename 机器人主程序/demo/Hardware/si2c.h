/**
  ******************************************************************************
  * @file    si2c.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   软件IIC通信模块头文件
  ******************************************************************************
  */
  
#include "stm32f10x.h"

typedef struct
{
	GPIO_TypeDef *SCL_GPIOx; // SCL引脚的组编号
	uint16_t SCL_GPIO_Pin;   // SCL引脚的引脚编号
	
	GPIO_TypeDef *SDA_GPIOx; // SCL引脚的组编号
	uint16_t SDA_GPIO_Pin;   // SDA引脚的引脚编号
	
} SI2C_TypeDef;

void My_SI2C_Init(SI2C_TypeDef *SI2C);
int My_SI2C_SendBytes(SI2C_TypeDef *SI2C, uint8_t Addr, const uint8_t *pData, uint16_t Size);
int My_SI2C_ReceiveBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t *pBuffer, uint16_t Size);
int My_SI2C_RegReadBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t Reg, uint8_t *pBuffer, uint16_t Size);
int My_SI2C_RegWriteBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t Reg, const uint8_t *pData, uint16_t Size);
