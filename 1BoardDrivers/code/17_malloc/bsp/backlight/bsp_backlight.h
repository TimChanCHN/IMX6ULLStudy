#ifndef __BSP_BACKLIGHT_H
#define __BSP_BACKLIGHT_H
#include "imx6u.h"

/* 背光信息 */
struct backlight_dev_struc {
    unsigned char pwm_duty; /* 0～100 */
};

extern struct backlight_dev_struc backlight_dev;

void backlight_init(void);
void pwm1_setperiod_value(unsigned int value);
void pwm1_setduty(unsigned char duty);
void pwm1_irqhandler(unsigned int gicciar, void *param);
#endif