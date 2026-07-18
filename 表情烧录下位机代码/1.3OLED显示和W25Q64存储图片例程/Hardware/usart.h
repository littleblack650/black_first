#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdint.h>

// Functions
void USART1_Config(uint32_t baudrate);
void USART_SendBuffer(uint8_t *data, uint16_t length);
uint16_t USART_GetRxCount(void);
uint8_t USART_ReadByte(void);
void USART1_IRQHandler_Impl(void);

#endif
