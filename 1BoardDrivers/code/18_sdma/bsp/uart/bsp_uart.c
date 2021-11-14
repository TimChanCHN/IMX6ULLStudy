#include "bsp_uart.h"
#include "bsp_gpio.h"

enum _uart_sdma_transfer_states
{
    kUART_TxIdle,
    kUART_TxBusy,
    kUART_RxIdle,
    kUART_RxBusy,
};

static UART_Type *const s_uartBases[] = UART_BASE_PTRS;
static uart_sdma_private_handle_t s_sdmaPrivateHandle[UART_HANDLE_ARRAY_SIZE];

static void UART_ReceiveSDMACallback(sdma_handle_t *handle, void *param, bool transferDone, uint32_t tcds);
static void UART_SendSDMACallback(sdma_handle_t *handle, void *param, bool transferDone, uint32_t tcds);
void UART_TransferAbortSendSDMA(UART_Type *base, uart_sdma_handle_t *handle);
void UART_TransferAbortReceiveSDMA(UART_Type *base, uart_sdma_handle_t *handle);

/* 初始化UART1，波特率为115200 */
void uart_init(void)
{
    /* 初始化UART1的IO */
    uart_io_init();

    /* 初始化UART1 */
    uart_disable(UART1);    /* 关闭uart */
    uart_softreset(UART1);  /* 复位UART1 */

    /* 配置UART1 */
    UART1->UCR1 = 0;
    UART1->UCR1 &= ~(1<<14);

    /* 配置UART1的数据位、奇偶校验、停止位等等 */
    UART1->UCR2 = 0;
    UART1->UCR2 |= (1 << 1) | (1 << 2) | (1 << 5) | (1 << 14);

    UART1->UCR3 |= (1 << 2);


    /* 设置波特率为115200 */
    UART1->UFCR &= ~ ((7 << 7) | (0x3f << 0));  /* 对RFDIV进行清零 */
    UART1->UFCR |= (5 << 7) | (1 << 0);       /* 1分频，uart_clk=80MHz, RxFIFO water mask = 1 */
    UART1->UBIR = 71;
    UART1->UBMR = 3124;


#if 0
    uart_setbaudrate(UART1, 115200, 80000000);
#endif   
    /* 使能串口 */
    uart_enable(UART1);
}

/*
 * @description 		: 波特率计算公式，
 *    			  	  	  可以用此函数计算出指定串口对应的UFCR，
 * 				          UBIR和UBMR这三个寄存器的值
 * @param - base		: 要计算的串口。
 * @param - baudrate	: 要使用的波特率。
 * @param - srcclock_hz	:串口时钟源频率，单位Hz
 * @return		: 无
 */
void uart_setbaudrate(UART_Type *base, unsigned int baudrate, unsigned int srcclock_hz)
{
    uint32_t numerator = 0u;		//分子
    uint32_t denominator = 0U;		//分母
    uint32_t divisor = 0U;
    uint32_t refFreqDiv = 0U;
    uint32_t divider = 1U;
    uint64_t baudDiff = 0U;
    uint64_t tempNumerator = 0U;
    uint32_t tempDenominator = 0u;

    /* get the approximately maximum divisor */
    numerator = srcclock_hz;
    denominator = baudrate << 4;
    divisor = 1;

    while (denominator != 0)
    {
        divisor = denominator;
        denominator = numerator % denominator;
        numerator = divisor;
    }

    numerator = srcclock_hz / divisor;
    denominator = (baudrate << 4) / divisor;

    /* numerator ranges from 1 ~ 7 * 64k */
    /* denominator ranges from 1 ~ 64k */
    if ((numerator > (UART_UBIR_INC_MASK * 7)) || (denominator > UART_UBIR_INC_MASK))
    {
        uint32_t m = (numerator - 1) / (UART_UBIR_INC_MASK * 7) + 1;
        uint32_t n = (denominator - 1) / UART_UBIR_INC_MASK + 1;
        uint32_t max = m > n ? m : n;
        numerator /= max;
        denominator /= max;
        if (0 == numerator)
        {
            numerator = 1;
        }
        if (0 == denominator)
        {
            denominator = 1;
        }
    }
    divider = (numerator - 1) / UART_UBIR_INC_MASK + 1;

    switch (divider)
    {
        case 1:
            refFreqDiv = 0x05;
            break;
        case 2:
            refFreqDiv = 0x04;
            break;
        case 3:
            refFreqDiv = 0x03;
            break;
        case 4:
            refFreqDiv = 0x02;
            break;
        case 5:
            refFreqDiv = 0x01;
            break;
        case 6:
            refFreqDiv = 0x00;
            break;
        case 7:
            refFreqDiv = 0x06;
            break;
        default:
            refFreqDiv = 0x05;
            break;
    }
    /* Compare the difference between baudRate_Bps and calculated baud rate.
     * Baud Rate = Ref Freq / (16 * (UBMR + 1)/(UBIR+1)).
     * baudDiff = (srcClock_Hz/divider)/( 16 * ((numerator / divider)/ denominator).
     */
    tempNumerator = srcclock_hz;
    tempDenominator = (numerator << 4);
    divisor = 1;
    /* get the approximately maximum divisor */
    while (tempDenominator != 0)
    {
        divisor = tempDenominator;
        tempDenominator = tempNumerator % tempDenominator;
        tempNumerator = divisor;
    }
    tempNumerator = srcclock_hz / divisor;
    tempDenominator = (numerator << 4) / divisor;
    baudDiff = (tempNumerator * denominator) / tempDenominator;
    baudDiff = (baudDiff >= baudrate) ? (baudDiff - baudrate) : (baudrate - baudDiff);

    if (baudDiff < (baudrate / 100) * 3)
    {
        base->UFCR &= ~UART_UFCR_RFDIV_MASK;
        base->UFCR |= UART_UFCR_RFDIV(refFreqDiv);
        base->UBIR = UART_UBIR_INC(denominator - 1); //要先写UBIR寄存器，然后在写UBMR寄存器，3592页 
        base->UBMR = UART_UBMR_MOD(numerator / divider - 1);
    }
}

/* 关闭UART */
void uart_disable(UART_Type *base)
{
    base->UCR1 &= ~ (1 << 0);
}

/* 打开UART */
void uart_enable(UART_Type *base)
{
    base->UCR1 |= (1 << 0);
}

static inline void UART_EnableTxDMA(UART_Type *base, bool enable)
{
    if (enable)
    {
        base->UCR1 |= UART_UCR1_TXDMAEN_MASK;
    }
    else
    {
        base->UCR1 &= ~UART_UCR1_TXDMAEN_MASK;
    }
}

static inline void UART_EnableRxDMA(UART_Type *base, bool enable)
{
    if (enable)
    {
        base->UCR1 |= UART_UCR1_RXDMAEN_MASK;
    }
    else
    {
        base->UCR1 &= ~UART_UCR1_RXDMAEN_MASK;
    }
}

/* 复位UART */
void uart_softreset(UART_Type *base)
{
    base->UCR2 &= ~(1 << 0);
    while((base->UCR2 & 0X1) == 0);
}

/* UART1的IO初始化 */
void uart_io_init(void)
{
    IOMUXC_SetPinMux(IOMUXC_UART1_TX_DATA_UART1_TX, 0); /*复用为UART1_TX */
    IOMUXC_SetPinMux(IOMUXC_UART1_RX_DATA_UART1_RX, 0); /*复用为UART1_RX */
   
    IOMUXC_SetPinConfig(IOMUXC_UART1_TX_DATA_UART1_TX, 0x10b0);
    IOMUXC_SetPinConfig(IOMUXC_UART1_RX_DATA_UART1_RX, 0x10b0);
}

/* 通过UART1发送一个字符 */
void putc(unsigned char c)
{
    while(((UART1->USR2 >> 3 ) & 0X01) == 0);
    UART1->UTXD = c;
}

/* 通过串口接收数据 */
unsigned char getc(void)
{
     while(((UART1->USR2) & 0X01) == 0); /* 等待有数据可以读取 */
     return UART1->URXD;
}

/* 通过串口发送一串字符 */
void puts(char *str)
{
    char *p = str;
    
    while(*p)
        putc(*p++);
}


/* raise函数，防止编译报错 */
void raise(int sig_nr)
{

}

uint32_t UART_GetInstance(UART_Type *base)
{
    uint32_t instance;
    uint32_t uartArrayCount = (sizeof(s_uartBases) / sizeof(s_uartBases[0]));

    for (instance = 0; instance < uartArrayCount; instance++)
    {
        if (s_uartBases[instance] == base)
        {
            break;
        }
    }

    return instance;
}


void uart_transferCreateHandleSDMA(UART_Type *base, 
                                   uart_sdma_handle_t *handle, 
                                   uart_sdma_transfer_callback_t callback,
                                   void *userData, 
                                   sdma_handle_t *txSdmaHandle, 
                                   sdma_handle_t *rxSdmaHandle,
                                   uint32_t eventSourceTx, 
                                   uint32_t eventSourceRx)
{
    uint32_t instance = UART_GetInstance(base);
    memset(handle, 0, sizeof(*handle));

    handle->rxState = kUART_RxIdle;
    handle->txState = kUART_TxIdle;

    rxSdmaHandle->eventSource = eventSourceRx;
    txSdmaHandle->eventSource = eventSourceTx;

    handle->rxSdmaHandle = rxSdmaHandle;
    handle->txSdmaHandle = txSdmaHandle;

    handle->callback = callback;
    handle->userData = userData;

    s_sdmaPrivateHandle[instance].base = base;
    s_sdmaPrivateHandle[instance].handle = handle;

    if (txSdmaHandle)
    {
        SDMA_SetCallback(handle->txSdmaHandle, UART_SendSDMACallback, &s_sdmaPrivateHandle[instance]);
    }
    if (rxSdmaHandle)
    {
        SDMA_SetCallback(handle->rxSdmaHandle, UART_ReceiveSDMACallback, &s_sdmaPrivateHandle[instance]);
    }
}

static void UART_SendSDMACallback(sdma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
    uart_sdma_private_handle_t *uartPrivateHandle = (uart_sdma_private_handle_t *)param;

    if (transferDone)
    {
        UART_TransferAbortSendSDMA(uartPrivateHandle->base, uartPrivateHandle->handle);

        if (uartPrivateHandle->handle->callback)
        {
            uartPrivateHandle->handle->callback(uartPrivateHandle->base, uartPrivateHandle->handle,kStatus_UART_TxIdle, 
                                                uartPrivateHandle->handle->userData);
        }
    }
}

static void UART_ReceiveSDMACallback(sdma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
    uart_sdma_private_handle_t *uartPrivateHandle = (uart_sdma_private_handle_t *)param;

    if (transferDone)
    {
        UART_TransferAbortReceiveSDMA(uartPrivateHandle->base, uartPrivateHandle->handle);

        if (uartPrivateHandle->handle->callback)
        {
            uartPrivateHandle->handle->callback(uartPrivateHandle->base, uartPrivateHandle->handle, kStatus_UART_RxIdle,
                                                uartPrivateHandle->handle->userData);
        }
    }
}

void UART_TransferAbortSendSDMA(UART_Type *base, uart_sdma_handle_t *handle)
{
    // disable UART Tx SDMA
    UART_EnableTxDMA(base, false);

    // stop SDMA transfer
    SDMA_AbortTransfer(handle->txSdmaHandle);
    
    handle->txState = kUART_TxIdle;
}

void UART_TransferAbortReceiveSDMA(UART_Type *base, uart_sdma_handle_t *handle)
{
    UART_EnableRxDMA(base, false);

    SDMA_AbortTransfer(handle->rxSdmaHandle);

    handle->rxState = kUART_RxIdle;
}

/* UART Tx SDMA触发函数 */
status_t UART_SendSDMA(UART_Type *base, uart_sdma_handle_t *handle, uart_transfer_t *xfer)
{
    sdma_transfer_config_t xferConfig;
    status_t status;
    sdma_peripheral_t perType = kSDMA_PeripheralTypeUART;

    if (kUART_TxBusy == handle->txState)
    {
        status = kStatus_UART_TxBusy;
    }
    else
    {
        handle->txState = kUART_TxBusy;
        handle->txDataSizeAll = xfer->dataSize;

        uint32_t address = (uint32_t)base;

        if ((address >= FSL_FEATURE_SPBA_START) && (address < FSL_FEATURE_SPBA_END))
        {
            perType = kSDMA_PeripheralTypeUART_SP;
        }
        printf("123\r\n");
        SDMA_PrepareTransfer(&xferConfig, (uint32_t)xfer->data, (uint32_t)&(base->UTXD), sizeof(uint8_t),
                            sizeof(uint8_t), sizeof(uint8_t), xfer->dataSize, handle->txSdmaHandle->eventSource,
                            perType, kSDMA_MemoryToPeripheral);
        printf("123\r\n");
        SDMA_SubmitTransfer(handle->txSdmaHandle, &xferConfig);
        printf("123\r\n");
        SDMA_StartTransfer(handle->txSdmaHandle);
        printf("123\r\n");
        UART_EnableTxDMA(base, true);
        printf("123\r\n");
        status = kStatus_Success;
    }
    printf("status = %d\r\n", status);
    return status;
}

/* UART Rx SDMA触发函数 */
status_t UART_ReceiveSDMA(UART_Type *base, uart_sdma_handle_t *handle, uart_transfer_t *xfer)
{
    sdma_transfer_config_t xferConfig;
    status_t status;
    sdma_peripheral_t perType = kSDMA_PeripheralTypeUART;

    if (kUART_RxBusy == handle->rxState)
    {
        status = kStatus_UART_RxBusy;
    }
    else
    {
        handle->rxState = kUART_TxBusy;
        handle->rxDataSizeAll = xfer->dataSize;

        uint32_t address = (uint32_t)base;

        if ((address >= FSL_FEATURE_SPBA_START) && (address < FSL_FEATURE_SPBA_END))
        {
            perType = kSDMA_PeripheralTypeUART_SP;
        }

        SDMA_PrepareTransfer(&xferConfig, (uint32_t)&(base->URXD), (uint32_t)xfer->data, sizeof(uint8_t),
                            sizeof(uint8_t), sizeof(uint8_t), xfer->dataSize, handle->rxSdmaHandle->eventSource,
                            perType, kSDMA_MemoryToPeripheral);
        SDMA_SubmitTransfer(handle->rxSdmaHandle, &xferConfig);
        SDMA_StartTransfer(handle->rxSdmaHandle);
        UART_EnableRxDMA(base, true);
        status = kStatus_Success;
    }
    return status;
}

