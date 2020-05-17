#ifndef __BSP_KEYFILTER_H
#define __BSP_KEYFILTER_H
#include "imx6u.h"

void keyfilter_init(void);
void filtertimer_init(unsigned int value);
void filtertimer_stop(void);
void filtertimer_restart(unsigned int value);
void gpio1_16_31_irqhandler(unsigned int gicciar, void *param);
void filtertimer_irqhandler(unsigned int gicciar, void *param);
#endif
