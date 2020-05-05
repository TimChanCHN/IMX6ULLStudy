#include "imx6ul.h"
#include "bsp_clk.h"
#include "bsp_delay.h"
#include "bsp_led.h"
#include "bsp_beep.h"
#include "bsp_gpio.h"
#include "bsp_key.h"

/*
 * @description	: mian函数
 * @param 		: 无
 * @return 		: 无
 */
int main(void)
{
	int i = 0;
	int keyvalue = 0;
	unsigned char beep_state = OFF;
	unsigned char led_state = OFF;

	imx6u_clkinit();	/* 初始化系统时钟 */
	clk_enable();		/* 使能所有的时钟 			*/
	led_init();			/* 初始化led 			*/
	init_beep();		/* 初始化beep */
	init_key();			/* 初始化key */

	while(1)			
	{	
		keyvalue = get_key_value();
		if(keyvalue)
		{
			switch ((keyvalue))
			{
				case KEY0_VALUE:
					beep_state = !beep_state;
					beep_switch(beep_state);
					break;
			}
		}

		i++;
		if(i==50)
		{
			i = 0;
			led_state = !led_state;
			led_switch(LED0, led_state);
		}
		delay(10);
	}

	return 0;
}
