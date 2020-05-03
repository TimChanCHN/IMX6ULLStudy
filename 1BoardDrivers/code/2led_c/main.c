#include "main.h"

/* 初始化时钟 */
void init_clk(void)
{
    CCM_CCGR0 = 0xffffffff;
    CCM_CCGR1 = 0xffffffff;
    CCM_CCGR2 = 0xffffffff;
    CCM_CCGR3 = 0xffffffff;
    CCM_CCGR4 = 0xffffffff;
    CCM_CCGR5 = 0xffffffff;
    CCM_CCGR6 = 0xffffffff;
}

/* 初始化LED */
void init_led(void)
{
    SW_MUX_GPIO1_IO03 = 0x05;
    SW_PAD_GPIO1_IO03 = 0x10b0;

    GPIO1_GIDR = 0x08;
    GPIO1_DR = 0X0;
}

/* 短延时 */
void delay_short(unsigned int n)
{
    while(n--){}
}

/* 延时函数,该函数是延迟1ms */
void delay(unsigned int n)
{
    while(n--)
    {
        delay_short(0x7ff);
    }
}

void led_on()
{
    GPIO1_DR &= ~(1<<3);
}

void led_off()
{
    GPIO1_DR |= (1<<3);
}

int main(void)
{
    /* 1.初始化LED */
    init_clk();
    init_led();

    /* 2.设置LED闪烁 */
    while(1)
    {
        led_on();
        delay(500);
        led_off();
        delay(500);
    }
    return 0;
}