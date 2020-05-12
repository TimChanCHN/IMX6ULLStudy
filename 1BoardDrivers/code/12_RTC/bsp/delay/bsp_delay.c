
#include "bsp_delay.h"

/* 相关硬件初始化 */
void delay_init(void)
{
	GPT1->CR = 0; 					/* 清零，bit0也为0，即停止GPT  			*/

	GPT1->CR = 1 << 15;				/* bit15置1进入软复位 				*/
	while((GPT1->CR >> 15) & 0x01);	/*等待复位完成 						*/

	/* GPT的CR寄存器,GPT通用设置 */
	GPT1->CR = (1<<6);

	/* GPT的PR寄存器，GPT的分频设置 */
	GPT1->PR = 65;

	/* GPT的OCR1寄存器，GPT的输出比较1比较计数值 */
	GPT1->OCR[0] = 0XFFFFFFFF;
	GPT1->CR |= 1<<0;			//使能GPT1
}

/* us级延时 */
void delayus(unsigned int usdelay)
{
	unsigned long oldcnt,newcnt;
	unsigned long tcntvalue = 0;	/* 走过的总时间  */

	oldcnt = GPT1->CNT;
	while(1)
	{
		newcnt = GPT1->CNT;
		if(newcnt != oldcnt)
		{
			if(newcnt > oldcnt)		/* GPT是向上计数器,并且没有溢出 */
				tcntvalue += newcnt - oldcnt;
			else  					/* 发生溢出    */
				tcntvalue += 0XFFFFFFFF-oldcnt + newcnt;
			oldcnt = newcnt;
			if(tcntvalue >= usdelay)/* 延时时间到了 */
			break;			 		/*  跳出 */
		}
	}
}

/* ms级延时 */
void delayms(unsigned	 int msdelay)
{
	int i = 0;
	for(i=0; i<msdelay; i++)
	{
		delayus(1000);
	}
}


/*
 * @description	: 短时间延时函数
 * @param - n	: 要延时循环次数(空操作循环次数，模式延时)
 * @return 		: 无
 */
void delay_short(volatile unsigned int n)
{
	while(n--){}
}

/*
 * @description	: 延时函数,在396Mhz的主频下
 * 			  	  延时时间大约为1ms
 * @param - n	: 要延时的ms数
 * @return 		: 无
 */
void delay(volatile unsigned int n)
{
	while(n--)
	{
		delay_short(0x7ff);
	}
}


