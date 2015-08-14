#ifndef spi3080_h
 #define spi3080_h
#include "stm32f4xx.h"
#include "stm32f4xx_spi.h" 
#include "delay.h"

void SPI_init(int flag);
char SPI_SendReceive(char data);
#endif

