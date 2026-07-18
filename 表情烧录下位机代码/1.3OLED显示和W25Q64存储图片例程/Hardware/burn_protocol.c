#include "burn_protocol.h"
#include "W25Q64.h"
#include "usart.h"
#include <string.h>

// Buffer (increased to handle larger verification data)
#define BUFFER_SIZE 2048
static uint8_t rx_buffer[BUFFER_SIZE];
static uint8_t tx_buffer[BUFFER_SIZE];
static uint16_t rx_index = 0;
static uint32_t current_address = 0;

// Protocol Init
void Protocol_Init(void) {
    rx_index = 0;
    current_address = 0;
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(tx_buffer, 0, sizeof(tx_buffer));
}

// Calculate Checksum
uint8_t Protocol_CalculateChecksum(uint8_t *data, uint16_t length) {
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

// Verify Checksum
uint8_t Protocol_VerifyChecksum(uint8_t *data, uint16_t length) {
    uint8_t calculated = Protocol_CalculateChecksum(data, length - 1);
    return (calculated == data[length - 1]);
}

// Send Response
static void SendResponse(uint8_t status, uint8_t *data, uint16_t length) {
    ResponsePacket response;
    response.status = status;
    response.length = length;
    
    // Calculate Checksum
    uint8_t temp[sizeof(ResponsePacket) - 1];
    memcpy(temp, &response, sizeof(temp));
    response.checksum = Protocol_CalculateChecksum(temp, sizeof(temp));
    
    // Send Response Header
    USART_SendBuffer((uint8_t *)&response, sizeof(ResponsePacket));
    
    // Send Data (if any)
    if (length > 0 && data != NULL) {
        USART_SendBuffer(data, length);
    }
}

// Handle Erase Sector
static void HandleEraseSector(uint32_t address) {
    // Align to sector boundary
    uint32_t sector_addr = address & ~(W25Q64_SECTOR_SIZE - 1);
    
    // Erase Sector
    W25Q64_EraseSector(sector_addr);
    
    // Wait for erase complete
    while (W25Q64_IsBusy()) {
        // Wait
    }
    
    SendResponse(RESP_OK, NULL, 0);
}

// Handle Write Page
static void HandleWritePage(uint32_t address, uint8_t *data, uint16_t length) {
    if (length > W25Q64_PAGE_SIZE) {
        length = W25Q64_PAGE_SIZE;
    }
    
    // Write Data
    W25Q64_WritePage(address, data, length);
    
    // Wait for write complete
    while (W25Q64_IsBusy()) {
        // Wait
    }
    
    SendResponse(RESP_OK, NULL, 0);
}

// Handle Batch Write (requires address to be set first)
static void HandleBatchWrite(uint8_t *data, uint16_t length) {
    uint16_t offset = 0;
    uint16_t remaining = length;
    
    while (remaining > 0) {
        uint16_t chunk_size = (remaining > W25Q64_PAGE_SIZE) ? W25Q64_PAGE_SIZE : remaining;
        
        // Write current page
        W25Q64_WritePage(current_address, data + offset, chunk_size);
        
        // Wait for write complete
        while (W25Q64_IsBusy()) {
            // Wait
        }
        
        // Update address and offset
        current_address += chunk_size;
        offset += chunk_size;
        remaining -= chunk_size;
    }
    
    SendResponse(RESP_OK, NULL, 0);
}

// Handle Read Data
static void HandleReadData(uint32_t address, uint16_t length) {
    if (length > BUFFER_SIZE) {
        length = BUFFER_SIZE;
    }
    
    // Read Data
    W25Q64_ReadData(address, tx_buffer, length);
    
    SendResponse(RESP_OK, tx_buffer, length);
}

// Handle Verify Data
static void HandleVerifyData(uint32_t address, uint8_t *data, uint16_t length) {
    // Limit length to buffer size
    if (length > BUFFER_SIZE) {
        length = BUFFER_SIZE;
    }
    
    // Read Data from Flash
    W25Q64_ReadData(address, tx_buffer, length);
    
    // Compare Data
    if (memcmp(tx_buffer, data, length) == 0) {
        SendResponse(RESP_OK, NULL, 0);
    } else {
        SendResponse(RESP_ERROR, NULL, 0);
    }
}

// Handle Get Info
static void HandleGetInfo(void) {
    FlashInfo info;
    info.capacity = W25Q64_SIZE;
    info.sector_size = W25Q64_SECTOR_SIZE;
    info.page_size = W25Q64_PAGE_SIZE;
    info.sector_count = W25Q64_SIZE / W25Q64_SECTOR_SIZE;
    
    SendResponse(RESP_OK, (uint8_t *)&info, sizeof(FlashInfo));
}

// Protocol Process
void Protocol_Process(void) {
    // Read from USART receive buffer
    while (USART_GetRxCount() > 0) {
        uint8_t byte = USART_ReadByte();
        
        // Add byte to buffer
        if (rx_index < BUFFER_SIZE) {
            rx_buffer[rx_index++] = byte;
            
            // Check if complete command packet header received
            if (rx_index >= sizeof(CommandPacket)) {
                CommandPacket *cmd = (CommandPacket *)rx_buffer;
                
                // Verify checksum of the header
                if (Protocol_VerifyChecksum(rx_buffer, sizeof(CommandPacket))) {
                    // Determine if this command expects additional data (write/verify/batch)
                    uint8_t needs_data = (cmd->cmd == CMD_WRITE_PAGE || 
                                          cmd->cmd == CMD_VERIFY_DATA || 
                                          cmd->cmd == CMD_BATCH_WRITE);
                    
                    uint16_t total_expected = sizeof(CommandPacket) + (needs_data ? cmd->length : 0);
                    
                    // If command needs data but we haven't received all yet, continue
                    if (needs_data && rx_index < total_expected) {
                        continue;
                    }
                    
                    // Prepare data pointer and length for commands that have extra data
                    uint8_t *data_ptr = NULL;
                    uint16_t data_length = 0;
                    if (needs_data && cmd->length > 0) {
                        data_ptr = rx_buffer + sizeof(CommandPacket);
                        data_length = cmd->length;
                    }
                    
                    // Process command
                    switch (cmd->cmd) {
                        case CMD_ERASE_SECTOR:
                            HandleEraseSector(cmd->address);
                            break;
                            
                        case CMD_WRITE_PAGE:
                            HandleWritePage(cmd->address, data_ptr, data_length);
                            break;
                            
                        case CMD_BATCH_WRITE:
                            current_address = cmd->address;
                            HandleBatchWrite(data_ptr, data_length);
                            break;
                            
                        case CMD_READ_DATA:
                            // For read command, cmd->length is the number of bytes to read from flash
                            HandleReadData(cmd->address, cmd->length);
                            break;
                            
                        case CMD_VERIFY_DATA:
                            HandleVerifyData(cmd->address, data_ptr, data_length);
                            break;
                            
                        case CMD_GET_INFO:
                            HandleGetInfo();
                            break;
                            
                        case CMD_GET_VERSION:
                            {
                                const char version[] = "W25Q64_Burner_v1.0";
                                SendResponse(RESP_OK, (uint8_t *)version, sizeof(version) - 1);
                            }
                            break;
                            
                        default:
                            SendResponse(RESP_INVALID_CMD, NULL, 0);
                            break;
                    }
                } else {
                    SendResponse(RESP_CHECKSUM_ERROR, NULL, 0);
                }
                
                // Reset receive buffer after processing
                rx_index = 0;
            }
        } else {
            // Buffer overflow, reset
            rx_index = 0;
            SendResponse(RESP_ERROR, NULL, 0);
        }
    }
}
