#ifndef __SPI_H
#define __SPI_H

#include "stm32f10x.h"
#include <stddef.h>  // ÃÌº”NULL∂®“Â

void SPI1_Init(void);
void SPI_TransmitReceive(const uint8_t* txData, uint8_t* rxData, uint16_t size);

#endif /* __SPI_H */
