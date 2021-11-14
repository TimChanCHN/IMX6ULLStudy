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
#include "mymalloc.h"
#include "sdma.h"

#define BUFF_LENGTH 4U

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
sdma_handle_t g_SDMA_Handle = {0};

volatile bool g_Transfer_Done = false;
AT_NONCACHEABLE_SECTION_ALIGN(sdma_context_data_t context, 4) = {0};

AT_NONCACHEABLE_SECTION_ALIGN(uint32_t srcAddr[BUFF_LENGTH], 4) = {0x01, 0x02, 0x03, 0x04};
AT_NONCACHEABLE_SECTION_ALIGN(uint32_t destAddr[BUFF_LENGTH], 4) = {0x00, 0x00, 0x00, 0x00};

/*******************************************************************************
 * Code
 ******************************************************************************/

/* User callback function for SDMA transfer. */
void SDMA_Callback(sdma_handle_t *handle, void *param, bool transferDone, uint32_t bds)
{
    if (transferDone)
    {
        g_Transfer_Done = true;
    }
}

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
	unsigned char key = 0;
	unsigned char i = 0;
	unsigned char state = OFF;
	sdma_transfer_config_t transferConfig;
    sdma_config_t userConfig;

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
	// backlight_init();		/* 初始化背光*/

	tftlcd_dev.forecolor = LCD_RED;
	lcd_show_string(50, 10, 400, 24, 24, (char*)"IMX6U-ZERO BACKLIGHT TEST");  
	lcd_show_string(50, 40, 200, 16, 16, (char*)"BACKLIGHT TEST");  
	lcd_show_string(50, 60, 200, 16, 16, (char*)"ATOM@ALIENTEK");  
	lcd_show_string(50, 80, 200, 16, 16, (char*)"2019/3/27");  

	u8 *p = mymalloc(MEMPOOL_DDR, 2048);
	sprintf((char*)p, "this is malloc test");
	lcd_show_string(50, 110, 200, 16, 16, (char*)p);

    printf("\r\nSDMA memory to memory transfer example begin.\r\n");
    printf("\r\nDestination Buffer:\r\n");
    for (i = 0; i < BUFF_LENGTH; i++)
    {
        printf("%d\t", destAddr[i]);
    }

	/*    DMA初始化设置          */
    /* Configure SDMA one shot transfer */
    SDMA_GetDefaultConfig(&userConfig);
    SDMA_Init(SDMAARM, &userConfig);
    SDMA_CreateHandle(&g_SDMA_Handle, SDMAARM, 1, &context);
    SDMA_SetCallback(&g_SDMA_Handle, SDMA_Callback, NULL);
    SDMA_PrepareTransfer(&transferConfig, (uint32_t)srcAddr, (uint32_t)destAddr, sizeof(srcAddr[0]),
                         sizeof(destAddr[0]), sizeof(srcAddr[0]), sizeof(srcAddr), 0, kSDMA_PeripheralTypeMemory,
                         kSDMA_MemoryToMemory);
    SDMA_SubmitTransfer(&g_SDMA_Handle, &transferConfig);
    SDMA_SetChannelPriority(SDMAARM, 1, 2U);
    SDMA_StartTransfer(&g_SDMA_Handle);
    /* Wait for SDMA transfer finish */
    while (g_Transfer_Done != true)
    {
    }
    g_Transfer_Done = false;
    /* Print destination buffer */
    printf("\r\nSDMA memory to memory transfer example finish.\r\n");
    printf("Destination Buffer:\r\n");
    for (i = 0; i < BUFF_LENGTH; i++)
    {
        printf("%d\t", destAddr[i]);
    }

	while(1)
	{	
		key = key_getvalue();
		if(key == KEY0_VALUE) {
            printf("\r\nKEY0 is pressed. SDMA memory to memory transfer start!!!\r\n");
            printf("Src Buffer:\r\n");
            
            for (i = 0; i < BUFF_LENGTH; i++)
            {
                srcAddr[i] = srcAddr[i]*srcAddr[i];
                printf("%d\t", srcAddr[i]);
            }
            SDMA_SubmitTransfer(&g_SDMA_Handle, &transferConfig);
            SDMA_StartTransfer(&g_SDMA_Handle);
		}

        if (g_Transfer_Done == true)
        {
            g_Transfer_Done = false;
            printf("\r\n SDMA memory receive from the KEY0. Now is done.\r\n");
            printf("Destination Buffer:\r\n");
            for (i = 0; i < BUFF_LENGTH; i++)
            {
                printf("%d\t", destAddr[i]);
            }
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
