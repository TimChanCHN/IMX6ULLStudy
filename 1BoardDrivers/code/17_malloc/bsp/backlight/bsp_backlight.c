#include "bsp_backlight.h"
#include "bsp_int.h"

struct backlight_dev_struc backlight_dev;

/* 初始化背光 */
void backlight_init(void)
{
    unsigned char i = 0;

    /* 1、IO初始化 */
    IOMUXC_SetPinMux(IOMUXC_GPIO1_IO08_PWM1_OUT , 0); /*复用为PWM1_OUT */
    IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO08_PWM1_OUT , 0Xb090);

    /* 2、PWM初始化
     * PWM时钟源=66MHz，设置66分频，因此PWM时钟频率为1MHz
     * 
     */
    PWM1->PWMCR = 0;    /* 清零 */
    PWM1->PWMCR |= (1 << 16) | (65 << 4) | (1 << 26);


    pwm1_setperiod_value(1000);  /* PWM频率为1Khz */

    /* 3、设置默认站空比 */   
    backlight_dev.pwm_duty = 50;
    for(i = 0; i < 4; i++)
    {
        pwm1_setduty(backlight_dev.pwm_duty);
    }

    /*4、使能FIFO空中断  */
    PWM1->PWMIR = 1 << 0;   /* 使能PWM1的FIFO空中断 */
    GIC_EnableIRQ(PWM1_IRQn);
    system_register_irqhandler(PWM1_IRQn, (system_irq_handler_t)pwm1_irqhandler, NULL);
    PWM1->PWMSR = 0xff;


    /* 5、打开PWM */
    PWM1->PWMCR |= 1 << 0;


}

/* 设置PR寄存器 */
void pwm1_setperiod_value(unsigned int value)
{
    unsigned int regvalue = 0;

    if(value < 2) 
        regvalue = 2;
    else 
        regvalue = value - 2;
    PWM1->PWMPR = (regvalue & 0XFFFF);
}

/* 设置站空比duty=0~100 */
void pwm1_setduty(unsigned char duty)
{
    unsigned short period = 0;
    unsigned short sample = 0;

    backlight_dev.pwm_duty = duty;
    period = PWM1->PWMPR + 2;
    sample = (unsigned short)(period * backlight_dev.pwm_duty / 100.0f);
    PWM1->PWMSAR = (sample & 0xffff);
}

/* 中断处理函数 */
void pwm1_irqhandler(unsigned int gicciar, void *param)
{
    if(PWM1->PWMSR & (1 << 3)) /* FIFO空中断 */
    {
        pwm1_setduty(backlight_dev.pwm_duty);
        PWM1->PWMSR |= 1 << 3;  /* 中断标志位清零 */
    }
}