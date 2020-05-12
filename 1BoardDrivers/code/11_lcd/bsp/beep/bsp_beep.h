#ifndef __BSP_BEEP_H
#define __BSP_BEEP_H
#include "imx6ul.h"

#define BEEP    0

void init_beep(void);
void beep_ctrl(int beep, int status);
void beep_switch(int status);

#endif // !__BSP_BEEP_H
