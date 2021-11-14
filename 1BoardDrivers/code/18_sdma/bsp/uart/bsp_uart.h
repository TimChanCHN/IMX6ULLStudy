#ifndef __BSP_UART_H
#define __BSP_UART_H
#include "imx6u.h"
#include "sdma.h"
#include "string.h"

#define UART_HANDLE_ARRAY_SIZE 1
#define UART_TX_DMA_REQUEST     (26)
#define UART_RX_DMA_REQUEST     (25)

enum _uart_status
{
    kStatus_UART_TxBusy = MAKE_STATUS(kStatusGroup_IUART, 0),              /*!< Transmitter is busy. */
    kStatus_UART_RxBusy = MAKE_STATUS(kStatusGroup_IUART, 1),              /*!< Receiver is busy. */
    kStatus_UART_TxIdle = MAKE_STATUS(kStatusGroup_IUART, 2),              /*!< UART transmitter is idle. */
    kStatus_UART_RxIdle = MAKE_STATUS(kStatusGroup_IUART, 3),              /*!< UART receiver is idle. */
    kStatus_UART_TxWatermarkTooLarge = MAKE_STATUS(kStatusGroup_IUART, 4), /*!< TX FIFO watermark too large  */
    kStatus_UART_RxWatermarkTooLarge = MAKE_STATUS(kStatusGroup_IUART, 5), /*!< RX FIFO watermark too large  */
    kStatus_UART_FlagCannotClearManually =
        MAKE_STATUS(kStatusGroup_IUART, 6),                                /*!< UART flag can't be manually cleared. */
    kStatus_UART_Error = MAKE_STATUS(kStatusGroup_IUART, 7),               /*!< Error happens on UART. */
    kStatus_UART_RxRingBufferOverrun = MAKE_STATUS(kStatusGroup_IUART, 8), /*!< UART RX software ring buffer overrun. */
    kStatus_UART_RxHardwareOverrun = MAKE_STATUS(kStatusGroup_IUART, 9),   /*!< UART RX receiver overrun. */
    kStatus_UART_NoiseError = MAKE_STATUS(kStatusGroup_IUART, 10),         /*!< UART noise error. */
    kStatus_UART_FramingError = MAKE_STATUS(kStatusGroup_IUART, 11),       /*!< UART framing error. */
    kStatus_UART_ParityError = MAKE_STATUS(kStatusGroup_IUART, 12),        /*!< UART parity error. */
    kStatus_UART_BaudrateNotSupport =
        MAKE_STATUS(kStatusGroup_IUART, 13), /*!< Baudrate is not support in current clock source */
    kStatus_UART_BreakDetect = MAKE_STATUS(kStatusGroup_IUART, 14), /*!< Receiver detect BREAK signal */
};

typedef struct _uart_sdma_handle uart_sdma_handle_t;

typedef void (*uart_sdma_transfer_callback_t)(UART_Type *base, 
                                            uart_sdma_handle_t *handle,
                                            status_t status,
                                            void *userData);

typedef struct _uart_transfer
{
    uint8_t *data;
    size_t dataSize;
}uart_transfer_t;

struct _uart_sdma_handle
{
    uart_sdma_transfer_callback_t callback;
    void *userData;
    size_t rxDataSizeAll;
    size_t txDataSizeAll;
    sdma_handle_t *txSdmaHandle;
    sdma_handle_t *rxSdmaHandle;
    volatile uint8_t txState;
    volatile uint8_t rxState;
};

typedef struct _uart_sdma_private_handle
{
    UART_Type *base;
    uart_sdma_handle_t *handle;
}uart_sdma_private_handle_t;

void uart_init(void);
void uart_io_init(void);
void uart_disable(UART_Type *base);
void uart_enable(UART_Type *base);
void uart_softreset(UART_Type *base);
void putc(unsigned char c);
unsigned char getc(void);
void puts(char *str);
void uart_setbaudrate(UART_Type *base, unsigned int baudrate, unsigned int srcclock_hz);
void uart_transferCreateHandleSDMA(UART_Type *base, uart_sdma_handle_t *handle, uart_sdma_transfer_callback_t callback,
                                    void *userData, sdma_handle_t *txSdmaHandle, sdma_handle_t *rxSdmaHandle,
                                    uint32_t eventSourceTx, uint32_t eventSourceRx);
status_t UART_SendSDMA(UART_Type *base, uart_sdma_handle_t *handle, uart_transfer_t *xfer);
status_t UART_ReceiveSDMA(UART_Type *base, uart_sdma_handle_t *handle, uart_transfer_t *xfer);

#endif