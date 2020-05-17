#include "main.h"
#include "bsp_clk.h"
#include "bsp_led.h"
#include "bsp_delay.h"
#include "bsp_beep.h"
#include "bsp_key.h"
#include "bsp_int.h"
#include "bsp_exti.h"
#include "bsp_epit.h"
#include "bsp_keyfilter.h"
#include "bsp_uart.h"
#include "bsp_lcd.h"
#include "bsp_rtc.h"
#include "bsp_lcdapi.h"
#include "bsp_ap3216c.h"
#include "bsp_icm20608.h"
#include "bsp_ft5426.h"
#include "bsp_backlight.h"
#include "stdio.h"


/*
 * @description	: 使能I.MX6U的硬件NEON和FPU
 * @param 		: 无
 * @return 		: 无
 */
 void imx6ul_hardfpu_enable(void)
{
	uint32_t cpacr;
	uint32_t fpexc;

	/* 使能NEON和FPU */
	cpacr = __get_CPACR();
	cpacr = (cpacr & ~(CPACR_ASEDIS_Msk | CPACR_D32DIS_Msk))
		   |  (3UL << CPACR_cp10_Pos) | (3UL << CPACR_cp11_Pos);
	__set_CPACR(cpacr);

	fpexc = __get_FPEXC();
	fpexc |= 0x40000000UL;	
	__set_FPEXC(fpexc);
}


int main(void)
{
	unsigned char duty;
	unsigned char key = 0;
	unsigned char i = 0;
	unsigned char state = OFF;

	imx6ul_hardfpu_enable(); /* 开启硬件浮点运算及ENON */
	
    int_init();         /* 初始化中断 */
    imx6u_clkinit();    /* 初始化系统时钟 */
    delay_init();       /* 延时初始化 */
    uart_init();        /* 初始化串口 */
    clk_enable();       /* 使能外设时钟 */
    led_init();         /* 初始化LED */
    beep_init();        /* 初始化奉命器 */
    key_init();         /* 初始化按键 */
    lcd_init();         /* 初始化LCD */
	backlight_init();		/* 初始化背光*/

	tftlcd_dev.forecolor = LCD_RED;
	lcd_show_string(50, 10, 400, 24, 24, (char*)"IMX6U-ZERO BACKLIGHT TEST");  
	lcd_show_string(50, 40, 200, 16, 16, (char*)"BACKLIGHT TEST");  
	lcd_show_string(50, 60, 200, 16, 16, (char*)"ATOM@ALIENTEK");  
	lcd_show_string(50, 80, 200, 16, 16, (char*)"2019/3/27");  

	duty = 10;
	pwm1_setduty(duty);
	while(1)
	{	

		key = key_getvalue();
		if(key == KEY0_VALUE) {
			duty += 10;
			if(duty > 100)
				duty = 0;
			printf("PWM1 Duty = %d\r\n", duty);
			pwm1_setduty(duty);
		}

		delay_ms(10);
		i++;
	
		if(i == 50)
		{	
			i = 0;
			state = !state;
			led_switch(LED0,state); 
		}
	}
	return 0;
}