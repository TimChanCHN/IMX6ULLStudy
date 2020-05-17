#ifndef __BSP_BEEP_H
#define __BSP_BEEP_H
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"


/* 函数声明 */
void beep_init(void);
void beep_switch(int status);

#endif