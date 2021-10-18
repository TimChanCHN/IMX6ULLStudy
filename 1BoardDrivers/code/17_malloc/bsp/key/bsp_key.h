#ifndef __BSP_KEY_H
#define __BSP_KEY_H
#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"

/* 按键值 */
enum keyvalue{
    KEY_NONE = 0,
    KEY0_VALUE,
};

/* 函数声明 */
void key_init(void);
int read_key(void);
int key_getvalue(void);

#endif