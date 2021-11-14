#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"

void delay_init(void);
void delay_us(unsigned int usdelay);
void delay_ms(unsigned int msdelay);
void delay_short(volatile unsigned int n);
void delay(volatile unsigned int n);
void gpt1_irqhandler(unsigned int gicciar, void *param);
#endif
