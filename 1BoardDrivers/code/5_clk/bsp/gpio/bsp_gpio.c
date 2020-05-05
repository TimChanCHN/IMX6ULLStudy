#include "bsp_gpio.h"

/* GPIO初始化 */
void gpio_init(GPIO_Type* base, int pin, gpio_pin_config_t* config)
{
    if( config->direction == kGPIO_DigitalInput )
    {
        base->GDIR &= ~( 1 << pin );
    }
    else
    {
        base->GDIR |= 1 << pin;
        gpio_pinwrite(base, pin, config->outputLogic);              /* 输出默认电平 */
    }
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
