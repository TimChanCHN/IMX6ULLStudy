#include "bsp_beep.h"
#include "bsp_gpio.h"

/* 初始化BEEP对应的BEEP--GPIO5--IO1 */
void init_beep(void)
{
    gpio_pin_config_t beep_config;

    beep_config.direction = kGPIO_DigitalOutput;
    /* 1、初始化IO复用 */
	IOMUXC_SetPinMux(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01,0);		/* 复用为GPIO1_IO03 */

    /* 2、配置GPIO5_IO01的IO属性	 */
    IOMUXC_SetPinConfig(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01, 0X10B0);

    /* 3、初始化GPIO,GPIO5_IO01设置为输出*/
	// GPIO5->GDIR |= (1 << 1);	 
    gpio_init( GPIO5, 1, &beep_config );

	/* 4、设置GPIO5_IO01输出低电平，关闭BEEP */
	// GPIO5->DR &= ~(1 << 1);	
    gpio_pinwrite( GPIO5, 1, 1 );	

}

void beep_ctrl(int beep, int status)
{
    switch(beep)
    {
        case BEEP:
            if( status == ON )
                // GPIO5->DR |= (1<<1);
                gpio_pinwrite( GPIO5, 1, 1 );	
            else if( status == OFF )
                // GPIO5->DR &= ~(1<<1);
                gpio_pinwrite( GPIO5, 1, 0 );	
            break;
            
        default:
            break;
    }
}

void beep_switch(int status)
{
    if( status == ON )
    {
        GPIO5->DR &= ~(1 << 1);
    }
    else if( status == OFF )
    {
        GPIO5->DR |= (1 << 1);
    }
}
