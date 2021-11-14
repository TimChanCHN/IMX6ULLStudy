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
#define EXAMPLE_UART_SDMA_BASEADDR SDMAARM
#define UART_RX_DMA_CHANNEL (1U)
#define UART_TX_DMA_CHANNEL (2U)
#define BUFFER_LEN 8U
#define BD_QUEUE_SIZE 2U
#define BUFFER_TOTAL_LEN (BUFFER_LEN * BD_QUEUE_SIZE)
#define ECHO_BUFFER_LENGTH (8)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
AT_NONCACHEABLE_SECTION_ALIGN(sdma_handle_t g_uartTxSdmaHandle, 4) = {0};
AT_NONCACHEABLE_SECTION_ALIGN(sdma_handle_t g_uartRxSdmaHandle, 4) = {0};
AT_NONCACHEABLE_SECTION_ALIGN(uart_sdma_handle_t g_uartSdmaHandle, 4) = {0};

volatile bool g_Transfer_Done = false;
AT_NONCACHEABLE_SECTION_ALIGN(sdma_context_data_t context_Tx, 4) = {0};
AT_NONCACHEABLE_SECTION_ALIGN(sdma_context_data_t context_Rx, 4) = {0};

AT_NONCACHEABLE_SECTION_ALIGN(uart_transfer_t sendXfer, 4) = {0};
AT_NONCACHEABLE_SECTION_ALIGN(uart_transfer_t receiveXfer, 4) = {0};

AT_NONCACHEABLE_SECTION_ALIGN(uint8_t g_tipString[], 4) = 
"UART SDMA transfer example\r\nBoard receives 8 characters then sends them out\r\nNow please input:\r\n";

AT_NONCACHEABLE_SECTION_ALIGN(uint8_t g_txBuffer[ECHO_BUFFER_LENGTH], 4) = {0};
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t g_rxBuffer[ECHO_BUFFER_LENGTH], 4) = {0};

volatile bool rxBufferEmpty = true;
volatile bool txBufferFull = false;
volatile bool txOnGoing = false;
volatile bool rxOnGoing = false;

/*******************************************************************************
 * Code
 ******************************************************************************/

/* User callback function for SDMA transfer. */
void UART_SDMACallback(sdma_handle_t *handle, void *param, bool transferDone, uint32_t bds)
{
    if (transferDone)
    {
        printf("bds = %d\n\r", bds);
        g_Transfer_Done = true;
    }
}

void UART_UserCallback(UART_Type *base, uart_sdma_handle_t *handle, status_t status, void *userData)
{
    userData = userData;

    if (kStatus_UART_TxIdle == status)
    {
        txBufferFull = false;
        txOnGoing = false;
    }
    if (kStatus_UART_RxIdle == status)
    {
        rxBufferEmpty = false;
        rxOnGoing = false;
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
    sdma_config_t userConfig;
    uart_transfer_t xfer;

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

	/*    DMA初始化设置          */
    /* Configure SDMA one shot transfer */
    SDMA_GetDefaultConfig(&userConfig);
    SDMA_Init(EXAMPLE_UART_SDMA_BASEADDR, &userConfig);
    SDMA_CreateHandle(&g_uartTxSdmaHandle, EXAMPLE_UART_SDMA_BASEADDR, UART_TX_DMA_CHANNEL, &context_Tx);
    SDMA_CreateHandle(&g_uartRxSdmaHandle, EXAMPLE_UART_SDMA_BASEADDR, UART_RX_DMA_CHANNEL, &context_Rx);
    SDMA_SetChannelPriority(EXAMPLE_UART_SDMA_BASEADDR, UART_TX_DMA_CHANNEL, 3U);
    SDMA_SetChannelPriority(EXAMPLE_UART_SDMA_BASEADDR, UART_RX_DMA_CHANNEL, 4U);

    /* Create UART DMA handle. */
    uart_transferCreateHandleSDMA(UART1, &g_uartSdmaHandle, UART_UserCallback, NULL, &g_uartTxSdmaHandle, &g_uartRxSdmaHandle,
                                    UART_TX_DMA_REQUEST, UART_RX_DMA_REQUEST);

    xfer.data = g_tipString;
    xfer.dataSize = sizeof(g_tipString) - 1;
    txOnGoing = true;
    UART_SendSDMA(UART1, &g_uartSdmaHandle, &xfer);
    printf("start to send.\r\n");
    while (txOnGoing)
    {
    }
    sendXfer.data = g_txBuffer;
    sendXfer.dataSize = ECHO_BUFFER_LENGTH;
    receiveXfer.data = g_rxBuffer;
    receiveXfer.dataSize = ECHO_BUFFER_LENGTH;
    printf("Send 1st successful.\r\n");

    while (1)
    {
        if ((!rxOnGoing) && rxBufferEmpty)
        {
            rxOnGoing = true;
            UART_ReceiveSDMA(UART1, &g_uartSdmaHandle, &receiveXfer);
        }

        if ((!txOnGoing) && txBufferFull)
        {
            txOnGoing = true;
            UART_SendSDMA(UART1, &g_uartSdmaHandle, &sendXfer);
        }

        if ((!rxBufferEmpty) && (!txBufferFull))
        {
            memcpy(g_txBuffer, g_rxBuffer, ECHO_BUFFER_LENGTH);
            rxBufferEmpty = true;
            txBufferFull = true;
        }
    }

	return 0;
}
