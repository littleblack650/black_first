#include "w25q64.h"
#include "spi.h"
#include <stddef.h>  // 添加NULL定义头文件

// 私有函数声明
static void W25Q64_WaitBusy(void);
static void W25Q64_WriteEnable(void);

// 初始化W25Q64
void W25Q64_Init(void)
{
    // 初始化SPI外设
    SPI1_Init();
    
    // 设置片选引脚(PA15)为推挽输出
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    W25Q64_CS_HIGH();  // 初始取消选中
    
    // 读取ID验证连接
    uint32_t id = W25Q64_ReadID();
    // 可添加ID验证逻辑 (W25Q64 ID应为0xEF4017)
}

// 读取芯片ID
uint32_t W25Q64_ReadID(void)
{
    uint8_t cmd = W25Q64_CMD_READ_ID;
    uint8_t idBuf[3] = {0};
    
    W25Q64_CS_LOW();
    SPI_TransmitReceive(&cmd, NULL, 1);
    SPI_TransmitReceive(NULL, idBuf, 3);
    W25Q64_CS_HIGH();
    
    return ((uint32_t)idBuf[0] << 16) | ((uint32_t)idBuf[1] << 8) | idBuf[2];
}

// 等待芯片空闲
static void W25Q64_WaitBusy(void)
{
    uint8_t cmd = W25Q64_CMD_READ_STATUS1;
    uint8_t status;
    
    do {
        W25Q64_CS_LOW();
        SPI_TransmitReceive(&cmd, NULL, 1);
        SPI_TransmitReceive(NULL, &status, 1);
        W25Q64_CS_HIGH();
    } while(status & 0x01);  // 检查BUSY位
}

// 发送写使能命令
static void W25Q64_WriteEnable(void)
{
    uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;
    
    W25Q64_CS_LOW();
    SPI_TransmitReceive(&cmd, NULL, 1);
    W25Q64_CS_HIGH();
}

// 擦除指定扇区(4KB)
void W25Q64_EraseSector(uint32_t sectorAddr)
{
    // 确保地址是4KB对齐
    sectorAddr &= 0xFFFFF000;
    
    uint8_t cmd[4] = {
        W25Q64_CMD_SECTOR_ERASE,
        (uint8_t)(sectorAddr >> 16),
        (uint8_t)(sectorAddr >> 8),
        (uint8_t)(sectorAddr)
    };
    
    W25Q64_WriteEnable();
    
    W25Q64_CS_LOW();
    SPI_TransmitReceive(cmd, NULL, 4);
    W25Q64_CS_HIGH();
    
    W25Q64_WaitBusy();
}

// 写入单页数据(最大256字节)
void W25Q64_WritePage(uint32_t addr, const uint8_t* data, uint16_t len)
{
    // 确保不跨页写入
    uint16_t pageOffset = addr % W25Q64_PAGE_SIZE;
    if (len > (W25Q64_PAGE_SIZE - pageOffset)) {
        len = W25Q64_PAGE_SIZE - pageOffset;
    }
    
    uint8_t cmd[4] = {
        W25Q64_CMD_PAGE_PROGRAM,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    
    W25Q64_WriteEnable();
    
    W25Q64_CS_LOW();
    SPI_TransmitReceive(cmd, NULL, 4);  // 发送命令和地址
    SPI_TransmitReceive(data, NULL, len);  // 发送数据
    W25Q64_CS_HIGH();
    
    W25Q64_WaitBusy();
}

// 写入任意长度数据(自动处理跨页和擦除)
void W25Q64_WriteData(uint32_t addr, const uint8_t* data, uint32_t size)
{
    uint32_t bytesWritten = 0;
    uint32_t currentAddr = addr;
    
    // 计算需要擦除的扇区范围
    uint32_t startSector = addr / W25Q64_SECTOR_SIZE;
    uint32_t endSector = (addr + size - 1) / W25Q64_SECTOR_SIZE;
    
    // 擦除所有覆盖的扇区
    for (uint32_t sector = startSector; sector <= endSector; sector++) {
        W25Q64_EraseSector(sector * W25Q64_SECTOR_SIZE);
    }
    
    // 分页写入数据
    while (bytesWritten < size) {
        uint16_t chunk = size - bytesWritten;
        if (chunk > W25Q64_PAGE_SIZE) {
            chunk = W25Q64_PAGE_SIZE;
        }
        
        // 处理跨页边界
        uint16_t pageOffset = currentAddr % W25Q64_PAGE_SIZE;
        if (pageOffset + chunk > W25Q64_PAGE_SIZE) {
            chunk = W25Q64_PAGE_SIZE - pageOffset;
        }
        
        W25Q64_WritePage(currentAddr, data + bytesWritten, chunk);
        
        bytesWritten += chunk;
        currentAddr += chunk;
    }
}

// 读取数据
void W25Q64_ReadData(uint32_t addr, uint8_t* buffer, uint32_t size)
{
    uint8_t cmd[4] = {
        W25Q64_CMD_READ_DATA,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    
    W25Q64_CS_LOW();
    SPI_TransmitReceive(cmd, NULL, 4);  // 发送读命令和地址
    SPI_TransmitReceive(NULL, buffer, size);  // 读取数据
    W25Q64_CS_HIGH();
}

// 检查芯片是否忙碌
uint8_t W25Q64_IsBusy(void)
{
    uint8_t cmd = W25Q64_CMD_READ_STATUS1;
    uint8_t status;
    
    W25Q64_CS_LOW();
    SPI_TransmitReceive(&cmd, NULL, 1);
    SPI_TransmitReceive(NULL, &status, 1);
    W25Q64_CS_HIGH();
    
    return (status & 0x01);
}
