#ifndef __BSP_EPIT_H
#define __BSP_EPIT_H
#include "imx6ul.h"

/* 函数声明 */
void epit1_init(unsigned int frac, unsigned int value);
void epit1_irqhandler(void);


#endif // !__BSP_EPIT_H
