/**
  ******************************************************************************
  * @file    spi.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   SPI通信模块头文件
  ******************************************************************************
  */
#ifndef __SPI_H
#define __SPI_H

#include "stm32f10x.h"
#include <stddef.h>  // 添加NULL定义

void SPI1_Init(void);
uint8_t SPI_TransmitReceive(const uint8_t* txData, uint8_t* rxData, uint16_t size);

#endif /* __SPI_H */
