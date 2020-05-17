#ifndef _BSP_SPI_H
#define _BSP_SPI_H

#include "imx6ul.h"

/* 函数声明 */
void spi_init(ECSPI_Type *base);
unsigned char spich0_readwrite_byte(ECSPI_Type *base, unsigned char txdata);

#endif

