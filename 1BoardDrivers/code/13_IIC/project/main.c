#include "imx6ul.h"
#include "bsp_clk.h"
#include "bsp_delay.h"
#include "bsp_led.h"
#include "bsp_beep.h"
#include "bsp_gpio.h"
#include "bsp_key.h"
#include "bsp_int.h"
#include "bsp_exti.h"
#include "bsp_epit.h"
#include "bsp_key_filter.h"
#include "bsp_uart.h"
#include "bsp_lcd.h"
#include "bsp_lcdapi.h"
#include "bsp_rtc.h"
#include "bsp_ap3216c.h"
#include "stdio.h"



/* 背景颜色索引 */
unsigned int backcolor[10] = {
	LCD_BLUE, 		LCD_GREEN, 		LCD_RED, 	LCD_CYAN, 	LCD_YELLOW, 
	LCD_LIGHTBLUE, 	LCD_DARKBLUE, 	LCD_WHITE, 	LCD_BLACK, 	LCD_ORANGE

}; 


/*
 * @description	: mian函数
 * @param 		: 无
 * @return 		: 无
 */
int main(void)
{
	unsigned short ir, als, ps;
	unsigned char state = OFF;

	int_init();			/* 初始化中断 */
	imx6u_clkinit();	/* 初始化系统时钟 */
	clk_enable();		/* 使能所有的时钟 			*/
	led_init();			/* 初始化led 			*/
	init_beep();		/* 初始化beep */
	init_key();			/* 初始化key */
	delay_init();		/* 初始化延时 */
	uart_init();
	lcd_init();			/* 初始化LCD */
	rtc_init();			/* 初始化RTC */

	tftlcd_dev.forecolor = LCD_RED;	
	lcd_show_string(30, 50, 200, 16, 16, (char*)"ZERO-IMX6U IIC TEST");  
	lcd_show_string(30, 70, 200, 16, 16, (char*)"AP3216C TEST");  
	lcd_show_string(30, 90, 200, 16, 16, (char*)"ATOM@ALIENTEK");  
	lcd_show_string(30, 110, 200, 16, 16, (char*)"2019/3/26");  

	while(ap3216c_init())			/* 检测不到AP3216C */
	{
		lcd_show_string(30, 130, 200, 16, 16, (char*)"AP3216C Check Failed!");
		delayms(500);
		lcd_show_string(30, 130, 200, 16, 16, (char*)"Please Check!        ");
		delayms(500);
	}

	lcd_show_string(30, 130, 200, 16, 16, (char*)"AP3216C Ready!");  
    lcd_show_string(30, 160, 200, 16, 16, (char*)" IR:");	 
	lcd_show_string(30, 180, 200, 16, 16, (char*)" PS:");	
	lcd_show_string(30, 200, 200, 16, 16, (char*)"ALS:");	
	tftlcd_dev.forecolor = LCD_BLUE;	

	while(1)
	{
		ap3216c_readdata(&ir, &ps, &als);		/* 读取数据		  	*/
		lcd_shownum(30 + 32, 160, ir, 5, 16);	/* 显示IR数据 		*/
        lcd_shownum(30 + 32, 180, ps, 5, 16);	/* 显示PS数据 		*/
        lcd_shownum(30 + 32, 200, als, 5, 16);	/* 显示ALS数据 	*/ 
		delayms(120);
		state = !state;
		led_switch(LED0,state);	
	}

	return 0;
}
