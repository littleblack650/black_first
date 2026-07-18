/**
  ******************************************************************************
  * @file    OLED.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   OLED屏幕头文件
  ******************************************************************************
  */
  
#ifndef __OLED_H
#define __OLED_H 
#include "sys.h"
#include "stdlib.h"	

/***************根据自己需求更改****************/
#define OLED_SCL_PORT  			GPIOB
#define OLED_SCL_PIN				GPIO_Pin_10
#define OLED_SCL_GPIO_CLK   RCC_APB2Periph_GPIOB
#define OLED_SDA_PORT  			GPIOB
#define OLED_SDA_PIN				GPIO_Pin_11
#define OLED_SDA_GPIO_CLK   RCC_APB2Periph_GPIOB
/*********************END**********************/

#define OLED_SCL_Clr() GPIO_ResetBits(OLED_SCL_PORT,OLED_SCL_PIN)
#define OLED_SCL_Set() GPIO_SetBits(OLED_SCL_PORT,OLED_SCL_PIN)

#define OLED_SDA_Clr() GPIO_ResetBits(OLED_SDA_PORT,OLED_SDA_PIN)
#define OLED_SDA_Set() GPIO_SetBits(OLED_SDA_PORT,OLED_SDA_PIN)

#define OLED_CMD  0
#define OLED_DATA 1

// 原有函数声明...
void OLED_ClearPoint(u8 x,u8 y);
void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);
void OLED_I2C_Start(void);
void OLED_I2C_Stop(void);
void OLED_I2C_WaitAck(void);
void OLED_Send_Byte(u8 dat);
void OLED_WR_Byte(u8 dat,u8 mode);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_DrawLine(u8 x1,u8 y1,u8 x2,u8 y2,u8 mode);
void OLED_DrawCircle(u8 x,u8 y,u8 r);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1,u8 mode);
void OLED_ShowChar6x8(u8 x,u8 y,u8 chr,u8 mode);
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1,u8 mode);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1,u8 mode);
void OLED_ShowChinese(u8 x,u8 y,u8 num,u8 size1,u8 mode);
void OLED_ScrollDisplay(u8 num,u8 space,u8 mode);
void OLED_ShowPicture(u8 x,u8 y,u8 sizex,u8 sizey,u8 BMP[],u8 mode);
void OLED_Init(void);

// 新增：直接操作显存的函数（不自动刷新）
void DrawCharToGRAM(u8 x, u8 y, u8 chr, u8 size1, u8 mode);
void DrawStringToGRAM(u8 x, u8 y, u8 *str, u8 size1, u8 mode);

#endif
