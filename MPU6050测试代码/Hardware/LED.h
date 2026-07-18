#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

/**
  * @brief  初始化板载LED（PC13）
  */
void LED_Init(void);

/**
  * @brief  切换LED状态
  */
void LED_Toggle(void);

/**
  * @brief  打开LED
  */
void LED_On(void);

/**
  * @brief  关闭LED
  */
void LED_Off(void);

#endif /* __LED_H */
