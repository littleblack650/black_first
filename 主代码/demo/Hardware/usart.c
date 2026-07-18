/**
  ******************************************************************************
  * @file    usart.c
  * @author  生活中的小黑
  * @version V1.0.0
  * @date    2025年8月4日
  * @brief   串口通信模块，处理语音指令接收
  ******************************************************************************
  */
#include "usart.h"
#include "delay.h"
#include <string.h>

/* 私有变量声明 */
static VoiceCommand g_lastVoiceCmd = VOICE_CMD_NONE;
static uint8_t g_newVoiceCmdFlag = 0;
static VoiceCommandCallback g_voiceCallback = NULL;

/* 防重复和节流变量 */
static uint32_t g_lastCmdTime = 0;
#define CMD_PROCESS_INTERVAL 300  // 指令处理最小间隔300ms

/* 环形缓冲区 - 用于中断处理时缓存数据 */
#define UART_RX_BUFFER_SIZE 16
static uint8_t g_uartRxBuffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t g_uartRxWriteIndex = 0;
static volatile uint16_t g_uartRxReadIndex = 0;

/* 私有函数声明 */
static void ProcessVoiceCommand(uint8_t cmd);
static uint8_t IsBufferEmpty(void);
static uint8_t IsBufferFull(void);
static void PutBuffer(uint8_t data);
static uint8_t GetBuffer(void);

/**
 * @brief  USART1初始化函数
 * @note   使用PB6(TX)和PB7(RX)作为USART1引脚，启用重映射功能
 *         波特率9600，8位数据，1位停止位，无校验
 */
void USART1_Config(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    /* 1. 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    /* 2. 将USART1引脚重映射到PB6(TX)和PB7(RX) */
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

    /* 3. 配置TX(PB6)为复用推挽输出 */
    gpio.GPIO_Pin   = GPIO_Pin_6;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    /* 4. 配置RX(PB7)为浮空输入 */
    gpio.GPIO_Pin   = GPIO_Pin_7;
    gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &gpio);

    /* 5. 配置USART参数 */
    usart.USART_BaudRate            = 9600;  // 与SU-03T和JQ8900匹配
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart);

    /* 6. 配置USART1接收中断 - 较低优先级 */
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2;  // 抢占优先级
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    /* 7. 使能USART1接收中断 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    /* 8. 使能USART1 */
    USART_Cmd(USART1, ENABLE);
    
    /* 9. 初始化缓冲区 */
    g_uartRxWriteIndex = 0;
    g_uartRxReadIndex = 0;
}

/**
 * @brief  注册语音指令回调函数
 * @param  callback: 回调函数指针
 */
void USART_RegisterVoiceCallback(VoiceCommandCallback callback)
{
    g_voiceCallback = callback;
}

/**
 * @brief  获取最后接收到的语音指令
 * @retval 最后接收到的语音指令
 */
VoiceCommand USART_GetLastVoiceCommand(void)
{
    g_newVoiceCmdFlag = 0;  // 获取后清除标志
    return g_lastVoiceCmd;
}

/**
 * @brief  清除语音指令缓存
 */
void USART_ClearVoiceCommand(void)
{
    g_lastVoiceCmd = VOICE_CMD_NONE;
    g_newVoiceCmdFlag = 0;
}

/**
 * @brief  检查是否有新的语音指令
 * @retval 1:有新指令, 0:无指令
 */
uint8_t USART_HasNewVoiceCommand(void)
{
    return g_newVoiceCmdFlag;
}

/**
 * @brief  处理接收缓冲区中的所有数据，应在主循环中调用
 * @retval 1:处理了指令, 0:无指令处理
 */
uint8_t USART_ProcessVoiceData(void)
{
    static uint32_t lastProcessTime = 0;
    uint32_t currentTime = GetTick();
    
    // 控制处理频率，每50ms处理一次，避免影响其他任务
    if (currentTime - lastProcessTime < 50) {
        return 0;
    }
    lastProcessTime = currentTime;
    
    // 处理缓冲区中所有数据
    uint8_t processed = 0;
    while (!IsBufferEmpty()) {
        uint8_t cmd = GetBuffer();
        ProcessVoiceCommand(cmd);
        processed = 1;
    }
    
    return processed;
}

/* ========== 私有函数实现 ========== */

/**
 * @brief  处理语音指令（包含防重复过滤）
 * @param  cmd: 接收到的指令
 */
static void ProcessVoiceCommand(uint8_t cmd)
{
    uint32_t current_time = GetTick();
    
    /* 验证指令范围 - 允许从 VOICE_CMD_01 到 VOICE_CMD_MAX-1 */
    if (cmd < VOICE_CMD_01 || cmd >= VOICE_CMD_MAX) {
        #ifdef DEBUG_ENABLE
        printf("Invalid voice command: 0x%02X\n", cmd);
        #endif
        return;
    }
    
    /* 防重复：控制指令处理间隔 */
    if (current_time - g_lastCmdTime < CMD_PROCESS_INTERVAL) {
        #ifdef DEBUG_ENABLE
        printf("Command ignored (too frequent): 0x%02X\n", cmd);
        #endif
        return;
    }
    
    g_lastCmdTime = current_time;
    g_lastVoiceCmd = (VoiceCommand)cmd;
    g_newVoiceCmdFlag = 1;

    /* 调用回调函数通知上层 */
    if (g_voiceCallback != NULL) {
        #ifdef DEBUG_ENABLE
        printf("Processing voice command: 0x%02X\n", cmd);
        #endif
        g_voiceCallback(g_lastVoiceCmd);
    }
    
    #ifdef DEBUG_ENABLE
    printf("Voice command processed: 0x%02X\n", cmd);
    #endif
}


/* ========== 环形缓冲区操作 ========== */

/**
 * @brief  检查缓冲区是否为空
 * @retval 1:空, 0:非空
 */
static uint8_t IsBufferEmpty(void)
{
    return (g_uartRxReadIndex == g_uartRxWriteIndex);
}

/**
 * @brief  检查缓冲区是否已满
 * @retval 1:满, 0:未满
 */
static uint8_t IsBufferFull(void)
{
    return ((g_uartRxWriteIndex + 1) % UART_RX_BUFFER_SIZE == g_uartRxReadIndex);
}

/**
 * @brief  向缓冲区写入数据
 * @param  data: 要写入的数据
 */
static void PutBuffer(uint8_t data)
{
    if (!IsBufferFull()) {
        g_uartRxBuffer[g_uartRxWriteIndex] = data;
        g_uartRxWriteIndex = (g_uartRxWriteIndex + 1) % UART_RX_BUFFER_SIZE;
    }
    // 如果缓冲区满，数据会被丢弃（防止内存越界）
}

/**
 * @brief  从缓冲区读取数据
 * @retval 读取到的数据
 */
static uint8_t GetBuffer(void)
{
    uint8_t data = 0;
    if (!IsBufferEmpty()) {
        data = g_uartRxBuffer[g_uartRxReadIndex];
        g_uartRxReadIndex = (g_uartRxReadIndex + 1) % UART_RX_BUFFER_SIZE;
    }
    return data;
}

/* ========== 中断服务函数 ========== */

/**
 * @brief  USART1中断服务函数
 * @note   在stm32f10x_it.c中需要调用此函数作为中断处理
 *         这里只做简单的数据存储，确保中断处理时间短
 */
void USART1_IRQHandler_Impl(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        /* 读取接收到的数据 */
        uint8_t received_data = USART_ReceiveData(USART1);
        
        /* 存储到缓冲区，不进行复杂处理 */
        PutBuffer(received_data);
        
        /* 清除中断标志 */
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

/* ========== printf重定向 ========== */

#if defined(__CC_ARM)           /* Keil ARM Compiler */
int fputc(int ch, FILE *f)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
    return ch;
}
#elif defined(__GNUC__)         /* GCC / OpenOCD / STM32CubeIDE */
int __io_putchar(int ch)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
    return ch;
}
#endif
