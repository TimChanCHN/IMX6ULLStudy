#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

#include "imx6ul.h"


/* 函数声明 */
/* 函数声明 */
void delay_init(void);
void delayus(unsigned int usdelay);
void delayms(unsigned int msdelay);
void delay(volatile unsigned int n);
void gpt1_irqhandler(void);

#endif

