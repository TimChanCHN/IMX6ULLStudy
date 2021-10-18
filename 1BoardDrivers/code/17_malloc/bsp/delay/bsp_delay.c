#include "bsp_delay.h"
#include "bsp_int.h"
#include "bsp_led.h"

/* 延时初始化函数 */
void delay_init(void)
{
    GPT1->CR = 0;

    GPT1->CR = 1 << 15;
    while((GPT1->CR >> 15) & 0X01);

    /*
     * 设置GPT1时钟源为ipg_clk=66M，restart模式
     * 默认计数寄存器从0开始
     * 
     */
    GPT1->CR |= (1 << 1) | (1 << 6); 

    /* 分频设置 */
    GPT1->PR = 65;  /* 66分频，频率=66000000/66=1MHz */

    /*
     * 1M的频率，计1个数就是1us，那么0xffffffff=‭4294967295‬us=71.5min。
     */
    GPT1->OCR[0] = 0xffffffff;
#if 0
    /* 配置输出比较通道1 */
    GPT1->OCR[0] = 1000000/2;   /* 设置中断周期为500ms */

    /* 打开GPT1输出比较通道1中断 */
    GPT1->IR = 1 << 0;  

    /* 设置GIC */
    GIC_EnableIRQ(GPT1_IRQn);
    system_register_irqhandler(GPT1_IRQn, gpt1_irqhandler, NULL);

#endif
    GPT1->CR |= 1 << 0; /* 打开GPT1 */
}


#if 0
/* GPT1中断服务函数 */
void gpt1_irqhandler(unsigned int gicciar, void *param)
{
    static unsigned char state = 0;
    
    if(GPT1->SR & (1 << 0))
    {
        state =! state;
        led_switch(LED0, state);
    }

    GPT1->SR |= 1<<0; /* 清除中断标志位 */
}
#endif

/* us延时 */
void delay_us(unsigned int usdelay)
{
    unsigned long oldcnt,newcnt;
    unsigned long tcntvalue = 0;

    oldcnt = GPT1->CNT;
    while(1) {
        newcnt = GPT1->CNT;
        if(newcnt != oldcnt )
        {
            if(newcnt > oldcnt)
                tcntvalue += newcnt - oldcnt;
            else 
                tcntvalue += 0XFFFFFFFF - oldcnt + newcnt;
            oldcnt = newcnt;
            if(tcntvalue >= usdelay)
                break;
        }
    }
}

/* 毫秒延时 */
void delay_ms(unsigned int msdelay)
{
    int i = 0;
    for(i=0; i<msdelay; i++)
        delay_us(1000);
}

/* 短延时 */
void delay_short(volatile unsigned int n)
{
    while(n--){}
}

/* 延时，一次循环大概是1ms，在主频396MHz
 * n ：延时ms数
  */
void delay(volatile unsigned int n)
{
    while(n--) {
        delay_short(0x7ff);
    }
}