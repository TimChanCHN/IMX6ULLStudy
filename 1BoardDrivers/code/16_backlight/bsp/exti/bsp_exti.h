#ifndef __BSP_EXTI_H
#define __BSP_EXIT_H
#include "imx6u.h"

void exti_init(void);
void gpio1_io18_irqhandler(unsigned int gicciar, void *param);
#endif 
