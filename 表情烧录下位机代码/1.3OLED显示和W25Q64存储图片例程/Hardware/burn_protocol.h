#ifndef __BURN_PROTOCOL_H
#define __BURN_PROTOCOL_H

#include "stm32f10x.h"
#include <stdint.h>

// 命令定义
typedef enum {
    CMD_ERASE_SECTOR = 0x01,
    CMD_WRITE_PAGE = 0x02,
    CMD_READ_DATA = 0x03,
    CMD_VERIFY_DATA = 0x04,
    CMD_GET_INFO = 0x05,
    CMD_BATCH_WRITE = 0x08,
    CMD_GET_VERSION = 0x0E
} BurnCommand;

// 响应状态
typedef enum {
    RESP_OK = 0x00,
    RESP_ERROR = 0x01,
    RESP_INVALID_CMD = 0x04,
    RESP_CHECKSUM_ERROR = 0x07
} ResponseStatus;

// 命令包结构
#pragma pack(push, 1)
typedef struct {
    uint8_t cmd;
    uint32_t address;
    uint16_t length;
    uint8_t checksum;
} CommandPacket;

typedef struct {
    uint8_t status;
    uint16_t length;
    uint8_t checksum;
} ResponsePacket;

typedef struct {
    uint32_t capacity;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t sector_count;
} FlashInfo;
#pragma pack(pop)

// 函数声明
void Protocol_Init(void);
void Protocol_Process(void);
uint8_t Protocol_CalculateChecksum(uint8_t *data, uint16_t length);

#endif
