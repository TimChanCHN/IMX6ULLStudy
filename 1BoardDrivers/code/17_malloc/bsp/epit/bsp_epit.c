#include "bsp_epit.h"
#include "bsp_int.h"
#include "bsp_led.h"


/* 初始化EPIT1 */
void epit1_init(unsigned int frac, unsigned int value)
{
    if( frac > 4095 )
        frac = 4095;

    /* 配置EPIT1_CR寄存器 */
    EPIT1->CR = 0;

    EPIT1->CR = (1 << 1) | (1 << 2) | (1 << 3) | (frac << 4) | (1 << 24);

    EPIT1->LR = value; /* 加载寄存器，相当于倒计数值 */
    EPIT1->CMPR = 0;

    /* 初始化中断 */
    GIC_EnableIRQ(EPIT1_IRQn);
    system_register_irqhandler(EPIT1_IRQn, epit1_irqhandler, NULL);

    /* 打开EPIT1 */
    EPIT1->CR |= 1 << 0;
}

/* EPIT1中断服务函数 */
void epit1_irqhandler(unsigned int gicciar, void *param)
{
    static unsigned char state = 0;

    state = !state;
    if(EPIT1->SR & (1 << 0)) /* 中断发生了 */
    {
        led_switch(LED0, state); /* 反转LED灯 */
    }

    /* 清除中断标志位 */
    EPIT1->SR |= (1 << 0);
}