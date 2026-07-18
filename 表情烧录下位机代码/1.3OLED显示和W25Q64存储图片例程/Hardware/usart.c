#include "usart.h"
#include "delay.h"
#include <string.h>

/* Receive Buffer */
#define RX_BUFFER_SIZE 512
static uint8_t g_rxBuffer[RX_BUFFER_SIZE];
static volatile uint16_t g_rxWriteIndex = 0;
static volatile uint16_t g_rxReadIndex = 0;

/* Transmit Buffer */
#define TX_BUFFER_SIZE  2048   // ԭΪ 256
static uint8_t g_txBuffer[TX_BUFFER_SIZE];
static uint16_t g_txWriteIndex = 0;
static uint16_t g_txReadIndex = 0;
static volatile uint8_t g_txBusy = 0;

/* Private Functions */
static uint8_t IsRxBufferEmpty(void);
static uint8_t IsRxBufferFull(void);
static void PutRxBuffer(uint8_t data);
static uint8_t GetRxBuffer(void);
static uint16_t GetRxAvailable(void);
static uint8_t IsTxBufferEmpty(void);
static uint8_t IsTxBufferFull(void);
static void PutTxBuffer(uint8_t data);
static uint8_t GetTxBuffer(void);
static void SendNextByte(void);

/*
 * @brief  USART1 Configuration
 * @param  baudrate: Baudrate
 */
void USART1_Config(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    /* 1. Enable Clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    /* 2. Remap USART1 to PB6(TX) and PB7(RX) */
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

    /* 3. Configure TX(PB6) as Alternate Function Push-Pull */
    gpio.GPIO_Pin   = GPIO_Pin_6;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    /* 4. Configure RX(PB7) as Input Floating */
    gpio.GPIO_Pin   = GPIO_Pin_7;
    gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &gpio);

    /* 5. Configure USART Parameters */
    usart.USART_BaudRate            = baudrate;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart);

    /* 6. Configure USART1 Interrupt */
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    /* 7. Enable USART1 Receive and Transmit Complete Interrupts */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TC, ENABLE);
    
    /* 8. Enable USART1 */
    USART_Cmd(USART1, ENABLE);
    
    /* 9. Initialize Buffers */
    g_rxWriteIndex = 0;
    g_rxReadIndex = 0;
    g_txWriteIndex = 0;
    g_txReadIndex = 0;
    g_txBusy = 0;
    
    /* 10. Send Welcome Message */
    const char* welcome = "\r\n=== W25Q64 Burner Ready ===\r\n";
    for (uint8_t i = 0; i < strlen(welcome); i++) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, (uint16_t)welcome[i]);
    }
}

/*
 * @brief  Send Data Buffer
 * @param  data: Data pointer
 * @param  length: Data length
 */
void USART_SendBuffer(uint8_t *data, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        // Wait for transmit buffer to be ready
        while (IsTxBufferFull()) {
            Delay_us(10);
        }
        
        PutTxBuffer(data[i]);
    }
    
    // If transmit is idle, start sending
    if (!g_txBusy) {
        SendNextByte();
    }
}

/*
 * @brief  Get Receive Buffer Available Count
 * @retval Available byte count
 */
uint16_t USART_GetRxCount(void)
{
    return GetRxAvailable();
}

/*
 * @brief  Read One Byte from Receive Buffer
 * @retval Read byte
 */
uint8_t USART_ReadByte(void)
{
    return GetRxBuffer();
}

/*
 * @brief  USART1 Interrupt Handler Implementation
 */
void USART1_IRQHandler_Impl(void)
{
    /* Receive Interrupt */
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t received_data = USART_ReceiveData(USART1);
        
        // Store in receive buffer
        if (!IsRxBufferFull()) {
            PutRxBuffer(received_data);
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
    
    /* Transmit Complete Interrupt */
    if (USART_GetITStatus(USART1, USART_IT_TC) != RESET) {
        // Check if transmit buffer has more data, send next byte
        if (!IsTxBufferEmpty()) {
            uint8_t nextByte = GetTxBuffer();
            USART_SendData(USART1, (uint16_t)nextByte);
        } else {
            g_txBusy = 0;  // Transmission complete
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_TC);
    }
}

/* ========== Buffer Management Functions ========== */

static uint8_t IsRxBufferEmpty(void)
{
    return (g_rxReadIndex == g_rxWriteIndex);
}

static uint8_t IsRxBufferFull(void)
{
    return ((g_rxWriteIndex + 1) % RX_BUFFER_SIZE == g_rxReadIndex);
}

static void PutRxBuffer(uint8_t data)
{
    if (!IsRxBufferFull()) {
        g_rxBuffer[g_rxWriteIndex] = data;
        g_rxWriteIndex = (g_rxWriteIndex + 1) % RX_BUFFER_SIZE;
    }
}

static uint8_t GetRxBuffer(void)
{
    uint8_t data = 0;
    if (!IsRxBufferEmpty()) {
        data = g_rxBuffer[g_rxReadIndex];
        g_rxReadIndex = (g_rxReadIndex + 1) % RX_BUFFER_SIZE;
    }
    return data;
}

static uint16_t GetRxAvailable(void)
{
    if (g_rxWriteIndex >= g_rxReadIndex) {
        return g_rxWriteIndex - g_rxReadIndex;
    } else {
        return RX_BUFFER_SIZE - g_rxReadIndex + g_rxWriteIndex;
    }
}

static uint8_t IsTxBufferEmpty(void)
{
    return (g_txReadIndex == g_txWriteIndex);
}

static uint8_t IsTxBufferFull(void)
{
    return ((g_txWriteIndex + 1) % TX_BUFFER_SIZE == g_txReadIndex);
}

static void PutTxBuffer(uint8_t data)
{
    if (!IsTxBufferFull()) {
        g_txBuffer[g_txWriteIndex] = data;
        g_txWriteIndex = (g_txWriteIndex + 1) % TX_BUFFER_SIZE;
    }
}

static uint8_t GetTxBuffer(void)
{
    uint8_t data = 0;
    if (!IsTxBufferEmpty()) {
        data = g_txBuffer[g_txReadIndex];
        g_txReadIndex = (g_txReadIndex + 1) % TX_BUFFER_SIZE;
    }
    return data;
}

static void SendNextByte(void)
{
    if (!IsTxBufferEmpty()) {
        uint8_t byte = GetTxBuffer();
        USART_SendData(USART1, (uint16_t)byte);
        g_txBusy = 1;
    }
}
