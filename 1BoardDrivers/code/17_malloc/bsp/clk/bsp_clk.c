#include "bsp_clk.h"

/* 使能外设时钟 */
void clk_enable(void)
{
    CCM->CCGR0 = 0xFFFFFFFF;
    CCM->CCGR1 = 0xFFFFFFFF;
    CCM->CCGR2 = 0xFFFFFFFF;
    CCM->CCGR3 = 0xFFFFFFFF;
    CCM->CCGR4 = 0xFFFFFFFF;
    CCM->CCGR5 = 0xFFFFFFFF;
    CCM->CCGR6 = 0xFFFFFFFF;
}

/* 初始化时钟 */
void imx6u_clkinit(void)
{
    unsigned int reg = 0;
    /* 初始化6U的主频为528MHz */
    if(((CCM->CCSR >> 2) & 0X1) == 0) /* 当前时钟使用pll1_main_clk也就是PLL1 */
    {
        CCM->CCSR &= ~(1 << 8); /* 设置step_clk = osc_clk=24M */
        CCM->CCSR |= (1 << 2);  /* pll1_sw_clk=step_clk=24MHz */
    }

    /* 设置PLL1=1056MHz */
    CCM_ANALOG->PLL_ARM = (1 << 13) | ((88 << 0) & 0x7f);
    CCM->CACRR = 1; /* 设置2分频 */
    CCM->CCSR &= ~(1 << 2); /* 设置pll1_sw_clk=pll1_main_clk=1056MHz */

    /* 设置PLL2的4路PFD */
    reg = CCM_ANALOG->PFD_528;
    reg &= ~(0x3F3F3F3F);
    reg |= (32 << 24);                    /* PLL2_PFD3=297MHz*/
    reg |= (24 << 16);                     /* PLL2_PFD2=396MHz*/
    reg |= (16 << 8);                     /* PLL2_PFD1=594MHz*/
    reg |= (27 << 0);                      /* PLL2_PFD0=352MHz*/
    CCM_ANALOG->PFD_528 = reg;

    /* 设置PLL3的4路PFD */
    reg = 0;
    reg = CCM_ANALOG->PFD_480;
    reg &= ~(0x3F3F3F3F);
    reg |= (19 << 24);                    /* PLL3_PFD3=454.7MHz*/
    reg |= (17 << 16);                     /* PLL3_PFD2=508.2MHz*/
    reg |= (16 << 8);                     /* PLL3_PFD1=540MHz*/
    reg |= (12 << 0);                      /* PLL3_PFD0=720MHz*/
    CCM_ANALOG->PFD_480 = reg;


    /* 设置AHB_CLK_ROOT=132MHz */
    CCM->CBCMR &= ~(3 << 18);
    CCM->CBCMR |= 1 << 18;  /* 设置pre_periph clock=PLL2_PFD2=396MHZ */
    CCM->CBCDR &= ~(1 << 25); 
    while(CCM->CDHIPR & (1 << 5)); /* 等待握手信号 */

#if 0
    CCM->CBCDR &= ~(7 << 10);
    CCM->CBCDR |= (2 << 10);    /* 3分频率 */
    while(CCM->CDHIPR & (1 << 1)); /* 等待握手信号 */
#endif

    /* IPG_CLK_ROOT=66MHz */
    CCM->CBCDR &= ~(3 << 8);
    CCM->CBCDR |= (1 << 8); /* IPG_CLK_ROOT=AHB_CLK_ROOT/2=132/2=66MHz */

    /* PERCLK_CLK_ROOT=66MHz */
    CCM->CSCMR1 &= ~(1 << 6);   /* PERCLK_CLK_ROOT时钟源为IPG_CLK=66mhZ */
    CCM->CSCMR1 &= ~(0x3f << 0); /* 1分频，PERCLK_CLK_ROOT=66mHZ */
    
    /* 设置UART根时钟为pll3_80M=80M */
    CCM->CSCDR1 &= ~ (1 << 6);  /* UART_CLK_ROOT=80M */
    CCM->CSCDR1 &= ~0X3F;       /* 1分频 */

    /* 设置ECSPI根时钟为PLL3_60MHz */
    CCM->CSCDR2 &= ~(1 << 18);  /* ECSPI时钟源为PLL3_60M */
    CCM->CSCDR2 &= ~(0X3F << 19); /* 1分频 */
}
