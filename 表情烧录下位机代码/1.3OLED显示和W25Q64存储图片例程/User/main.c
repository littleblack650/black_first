#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "W25Q64.h"
#include "usart.h"
#include "burn_protocol.h"
#include <stdio.h>

int main(void) {
    // System Init
    SystemInit();
    Delay_Init();
    OLED_Init();
    OLED_Clear();
    W25Q64_Init();
    USART1_Config(115200);
    Protocol_Init();
    
    // Display Init Message
    OLED_ShowString(0, 0, "W25Q64 Burner", 16, 1);
    OLED_ShowString(0, 16, "Baud: 115200", 16, 1);
    OLED_ShowString(0, 32, "Status: Ready", 16, 1);
    
    // Get Flash ID
    uint32_t flash_id = W25Q64_ReadID();
    char id_str[32];
    sprintf(id_str, "ID: 0x%06lX", flash_id);
    OLED_ShowString(0, 48, (u8*)id_str, 16, 1);
    
    while (1) {
        // Process Protocol
        Protocol_Process();
        
        Delay_ms(1);
		
    }
}
