#include "bsp_gpio.h"

/* GPIO初始化 */
void gpio_init(GPIO_Type* base, int pin, gpio_pin_config_t* config)
{
    base->IMR &= ~(1U << pin);

    if( config->direction == kGPIO_DigitalInput )
    {
        base->GDIR &= ~( 1 << pin );
    }
    else
    {
        base->GDIR |= 1 << pin;
        gpio_pinwrite(base, pin, config->outputLogic);              /* 输出默认电平 */
    }
    gpio_intconfig(base, pin, config->interruptMode);
}

/* 读取指定GPIO的电平 */
int gpio_pinread(GPIO_Type* base, int pin)
{
    return (((base->DR) >> pin) & 0x01);
}

/* 指定GPIO输出H/L电平 */
void gpio_pinwrite(GPIO_Type* base, int pin, int value)
{
    if ( value == 0U )
    {
        base->DR &= ~( 1U << pin );
    }
    else
    {
        base->DR |= ( 1U << pin );
    }
}

/* 设置GPIO中断配置 */
void gpio_intconfig(GPIO_Type* base, unsigned int pin, gpio_interrupt_mode_t pin_int_mode)
{
    volatile uint32_t *icr;
	uint32_t icrShift;

	icrShift = pin;
	
	base->EDGE_SEL &= ~(1U << pin);

	if(pin < 16) 	/* 低16位 */
	{
		icr = &(base->ICR1);
	}
	else			/* 高16位 */
	{
		icr = &(base->ICR2);
		icrShift -= 16;
	}
	switch(pin_int_mode)
	{
		case(kGPIO_IntLowLevel):
			*icr &= ~(3U << (2 * icrShift));                /* 每个寄存器占2个bit */
			break;
		case(kGPIO_IntHighLevel):
			*icr = (*icr & (~(3U << (2 * icrShift)))) | (1U << (2 * icrShift));
			break;
		case(kGPIO_IntRisingEdge):
			*icr = (*icr & (~(3U << (2 * icrShift)))) | (2U << (2 * icrShift));
			break;
		case(kGPIO_IntFallingEdge):
			*icr |= (3U << (2 * icrShift));
			break;
		case(kGPIO_IntRisingOrFallingEdge):
			base->EDGE_SEL |= (1U << pin);
			break;
		default:
			break;
	}
}

/* 使能GPIO中断 */
void gpio_enableint(GPIO_Type* base, unsigned int pin)
{
    base->IMR |= ( 1U << pin );
}

/* 禁止GPIO中断 */
void gpio_disableint(GPIO_Type* base, unsigned int pin)
{
    base->IMR &= ~(1 << pin);
}

/* 清除中断标志 */
void gpio_clearintflags(GPIO_Type* base, unsigned int pin)
{
    base->ISR |= (1 << pin);
}
