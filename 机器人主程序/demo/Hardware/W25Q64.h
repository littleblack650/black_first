/**
  ******************************************************************************
  * @file    W25Q64.h
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   W25Q64 Flash存储器驱动头文件
  ******************************************************************************
  */
#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"
#include <stdint.h>  // 添加标准整数类型头文件

// W25Q64 容量定义
#define W25Q64_SIZE          (8*1024*1024)  // 8MB容量
#define W25Q64_SECTOR_SIZE   4096           // 扇区大小4KB
#define W25Q64_PAGE_SIZE     256            // 页大小256字节

// W25Q64 指令集
#define W25Q64_CMD_WRITE_ENABLE  0x06
#define W25Q64_CMD_SECTOR_ERASE  0x20
#define W25Q64_CMD_PAGE_PROGRAM  0x02
#define W25Q64_CMD_READ_DATA     0x03
#define W25Q64_CMD_READ_STATUS1  0x05
#define W25Q64_CMD_READ_ID       0x9F

// 片选控制宏
#define W25Q64_CS_LOW()   GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_RESET)
#define W25Q64_CS_HIGH()  GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_SET)

// 函数声明
void W25Q64_Init(void);
void W25Q64_EraseSector(uint32_t sectorAddr);
void W25Q64_WritePage(uint32_t addr, const uint8_t* data, uint16_t len);
void W25Q64_WriteData(uint32_t addr, const uint8_t* data, uint32_t size);
uint8_t W25Q64_ReadData(uint32_t addr, uint8_t* buffer, uint32_t size);  // 返回0成功
uint32_t W25Q64_ReadID(void);
uint8_t W25Q64_IsBusy(void);

#endif /* __W25Q64_H */
