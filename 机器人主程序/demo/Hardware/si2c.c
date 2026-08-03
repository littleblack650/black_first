/**
  ******************************************************************************
  * @file    si2c.c
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   软件IIC通信模块
  ******************************************************************************
  */
  
#include "si2c.h"

#define scl_w(v) GPIO_WriteBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, ((v)?Bit_SET:Bit_RESET))
#define sda_w(v) GPIO_WriteBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, ((v)?Bit_SET:Bit_RESET))
#define scl_r ((GPIO_ReadInputDataBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin) == Bit_SET) ? 1 : 0)
#define sda_r ((GPIO_ReadInputDataBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin) == Bit_SET) ? 1 : 0)
void delay(uint32_t us) { for(uint32_t i = 0; i<8*us; i++); }

static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte);
static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack);
static void SendStop(SI2C_TypeDef *SI2C);


//
// @简介：对软件I2C进行初始化
//
__weak void My_SI2C_Init(SI2C_TypeDef *SI2C)
{
	// #1. 使能SCL引脚的时钟
	if(SI2C->SCL_GPIOx == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if(SI2C->SCL_GPIOx == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if(SI2C->SCL_GPIOx == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if(SI2C->SCL_GPIOx == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}
	
	// #2. 对SCL和SDA写1
	GPIO_WriteBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, Bit_SET);
	GPIO_WriteBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, Bit_SET);
	
	// #2. 使能SDA引脚的时钟
	if(SI2C->SDA_GPIOx == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if(SI2C->SDA_GPIOx == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if(SI2C->SDA_GPIOx == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if(SI2C->SDA_GPIOx == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}
	
	// #3. 初始化SCL引脚为输出开漏
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.GPIO_Pin = SI2C->SCL_GPIO_Pin;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(SI2C->SCL_GPIOx, &GPIO_InitStruct);
	
	// #4. 初始化SDA引脚为输出开漏
	
	GPIO_InitStruct.GPIO_Pin = SI2C->SDA_GPIO_Pin;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(SI2C->SDA_GPIOx, &GPIO_InitStruct);
}

//
// @简介：通过软件I2C向从机写入多个字节
// 
// @参数 I2Cx：填写要操作的I2C的名称，可以是I2C1或I2C2
// @参数 Addr：填写从机的地址，左对齐 - A6 A5 A4 A3 A2 A1 A0 0
// @参数 pData：要发送的数据（数组）
// @参数 Size：要发送的数据的数量，以字节为单位
//
// @返回值：0 - 发送成功， -1 - 寻址失败， -2 - 数据被拒收
//
__weak int My_SI2C_SendBytes(SI2C_TypeDef *SI2C, uint8_t Addr, const uint8_t *pData, uint16_t Size)
{
	sda_w(1);
	scl_w(1);
	
	// #1. 发送起始位
	sda_w(0);
	delay(1);
	
	// #2. 发送从机地址+RW
	if(SendByte(SI2C, Addr & 0xfe) != 0)
	{
		SendStop(SI2C);
		return -1; // 寻址失败
	}
	
	// #3. 发送数据
	for(uint16_t i=0; i<Size; i++)
	{
		if(SendByte(SI2C, pData[i]) != 0)
		{
			SendStop(SI2C);
			return -2; // 数据被拒收
		}
	}
	
	// #4. 发送停止位
	SendStop(SI2C);
	
	return 0;
}

//
// @简介：通过软件I2C从从机读多个字节
// 
// @参数 I2Cx：填写要操作的I2C的名称，可以是I2C1或I2C2
// @参数 Addr：填写从机的地址，左对齐 - A6 A5 A4 A3 A2 A1 A0 0
// @参数 pBuffer：接收缓冲区（数组）
// @参数 Size：要读取的数据的数量，以字节为单位
//
// @返回值：0 - 发送成功， -1 - 寻址失败
//
__weak int My_SI2C_ReceiveBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t *pBuffer, uint16_t Size)
{
	sda_w(1);
	scl_w(1);
	
	// #1. 发送起始位
	sda_w(0);
	delay(1);
	
	// #2. 发送从机地址+RW
	if(SendByte(SI2C, Addr | 0x01) != 0)
	{
		SendStop(SI2C);
		return -1; // 寻址失败
	}
	
	// #3. 接收
	for(uint16_t i=0; i<Size; i++)
	{
		pBuffer[i] = ReceiveByte(SI2C, (i==Size-1) ? 1 : 0);
	}
	
	// #4. 发送停止位
	SendStop(SI2C);
	
	return 0;
}

__weak int My_SI2C_RegReadBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t Reg, uint8_t *pBuffer, uint16_t Size)
{
	sda_w(1);
	scl_w(1);
	
	// #1. 发送起始位
	sda_w(0);
	delay(1);
	
	// #2. 发送从机地址+RW
	if(SendByte(SI2C, Addr & 0xfe) != 0)
	{
		SendStop(SI2C);
		return -1; // 寻址失败
	}
	
	// #3. 发送寄存器地址
	if(SendByte(SI2C, Reg) != 0)
	{
		SendStop(SI2C);
		return -2; // 数据被拒收
	}
	
	// #4. 发送重复起始位
	scl_w(0);
	sda_w(1);
	delay(1);
	scl_w(1);
	delay(1);
	sda_w(0);
	delay(1);
	
	// #5. 发送从机地址+RW
	if(SendByte(SI2C, Addr | 0x01) != 0)
	{
		SendStop(SI2C);
		return -1; // 寻址失败
	}
	
	// #6. 接收
	for(uint16_t i=0; i<Size; i++)
	{
		pBuffer[i] = ReceiveByte(SI2C, (i==Size-1) ? 0 : 1);
	}
	
	// #7. 发送停止位
	SendStop(SI2C);
	
	return 0;
}

__weak int My_SI2C_RegWriteBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t Reg, const uint8_t *pData, uint16_t Size)
{
	sda_w(1);
	scl_w(1);
	
	// #1. 发送起始位
	sda_w(0);
	delay(1);
	
	// #2. 发送从机地址+RW
	if(SendByte(SI2C, Addr & 0xfe) != 0)
	{
		SendStop(SI2C);
		return -1; // 寻址失败
	}
	
	// #3. 发送寄存器地址
	if(SendByte(SI2C, Reg) != 0)
	{
		SendStop(SI2C);
		return -2; // 数据被拒收
	}
	
	// #4. 发送数据
	for(uint16_t i=0; i<Size; i++)
	{
		if(SendByte(SI2C, pData[i]) != 0)
		{
			SendStop(SI2C);
			return -2; // 数据被拒收
		}
	}
	
	// #5. 发送停止位
	SendStop(SI2C);
	
	return 0;
}

//
// @简介：发送一个字节
//
// @返回值：0-ACK，其它-NAK
//
static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte)
{
	for(int8_t i=7; i>=0; i--)
	{
		scl_w(0); // 将SCL拉低
		sda_w((Byte & (0x01<<i)) ? 1 : 0); // 变SDA的电压
		delay(2); // 延迟1/2周期
		
		scl_w(1); // 将SCL拉高
		delay(2); // 延迟1/2周期
	}
	
	// 读取ACK
	scl_w(0); // 将SCL拉低
	sda_w(1); // 将SDA释放
	delay(2); // 延迟1/4周期
	
	scl_w(1); // 将SCL拉高
	delay(2); // 延迟1/4周期
	
	return sda_r;
}

//
// @简介：发送停止位
//
static void SendStop(SI2C_TypeDef *SI2C)
{
	scl_w(0); // scl拉低
	delay(1); // 延迟1/4周期
	sda_w(0); // sda拉低
	delay(1); // 延迟1/4周期
	scl_w(1); // scl拉高
	delay(1); // 延迟1/4周期
	sda_w(1); // sda拉高
	delay(1); // 延迟1/4周期
}


//
// @简介：从从机读取一个字节的数据
// @参数 Ack：0 - 回NAK，1 - 回ACK
// @返回值：读取到的数据
//
static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack)
{
	uint8_t ret = 0;
	
	for(int8_t i=7; i>=0; i--)
	{
		scl_w(0); // scl拉低
		sda_w(1); // 释放SDA
		delay(2); // 延迟1/2周期
		scl_w(1); // scl拉高
		delay(2); // 延迟1/2周期
		
		if(sda_r) // 如果读到的比特位为1
		{
			ret |= 0x01 << i; // 写入比特位
		}
		else // 如果读到的比特位为0
		{
			// 什么也不干
		}
	}
	
	// 回复ACK或NAK
	
	scl_w(0); // scl拉低
	
	if(Ack)
	{
		sda_w(0); // sda拉低
	}
	else
	{
		sda_w(1); // sda拉高
	}
	
	delay(2); // 延迟1/2周期
	
	return ret;
}
