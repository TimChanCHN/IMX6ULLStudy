#include "bsp_key.h"
#include "bsp_gpio.h"
#include "bsp_delay.h"

/* 初始化按键GPIO1_IO18 */
void init_key(void)
{
    gpio_pin_config_t key_config;

    key_config.direction = kGPIO_DigitalInput;
    /* 1、初始化IO复用 */
	IOMUXC_SetPinMux(IOMUXC_UART1_CTS_B_GPIO1_IO18,0);		/* 复用为GPIO1_IO03 */

    /* 2、配置GPIO1_IO18的IO属性	 */
    IOMUXC_SetPinConfig(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0XF080);

    /* 3、初始化GPIO,GPIO1_IO18设置为输出*/
    gpio_init( GPIO1, 18, &key_config );
}

/* 读取按键值 */
int get_key_value(void)
{
    int ret = 0;
    static unsigned char release = 1;       //松开标志

    if( (release == 1) && (gpio_pinread(GPIO1, 18) == 0) )
    {
        delay(10);
        release = 0;
        if( gpio_pinread(GPIO1, 18)==0 )
        {
            ret = KEY0_VALUE;
        }
    }
    else if( gpio_pinread(GPIO1, 18) == 1)
    {
        ret = 0;
        release = 1;
    }
    return ret;
}
