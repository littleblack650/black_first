#include "spi.h"

// SPI1初始化 (基于您原始代码)
void SPI1_Init(void)
{
    // #1. 初始化IO引脚
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB | RCC_APB2Periph_SPI1, ENABLE);
    
    // 重映射SPI1引脚
    GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // PB3 SCK AF_PP 50MHz
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // PB4 MISO Input Floating
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // PB5 MOSI AF_PP 50MHz
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // #2. 对SPI本身进行初始化
    SPI_InitTypeDef SPI_InitStruct;
    
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;  // 提高速度
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft; 
    SPI_InitStruct.SPI_CRCPolynomial = 7;
    
    SPI_Init(SPI1, &SPI_InitStruct);
    SPI_Cmd(SPI1, ENABLE);
}

// SPI传输函数 (优化版)
void SPI_TransmitReceive(const uint8_t* txData, uint8_t* rxData, uint16_t size)
{
    SPI_Cmd(SPI1, ENABLE);
    
    for(uint16_t i = 0; i < size; i++) {
        // 等待发送缓冲区空
        while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
        
        // 发送数据 (如果txData为NULL，发送0xFF)
        uint8_t txByte = txData ? txData[i] : 0xFF;
        SPI_I2S_SendData(SPI1, txByte);
        
        // 等待接收完成
        while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
        
        // 读取数据 (如果rxData不为NULL，保存数据)
        uint8_t rxByte = SPI_I2S_ReceiveData(SPI1);
        if(rxData) rxData[i] = rxByte;
    }
    
    SPI_Cmd(SPI1, DISABLE);
}
