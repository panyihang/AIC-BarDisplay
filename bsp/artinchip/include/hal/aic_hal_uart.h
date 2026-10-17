/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_HAL_UART_
#define _AIC_HAL_UART_

#include <aic_errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UART register bit definitions */

#define USR_UART_BUSY           0x01
#define USR_UART_TFE            0x04
#define USR_UART_RFNE           0x08
#define LSR_DATA_READY          0x01
#define LSR_THR_EMPTY           0x20
#define LSR_TEMT_EMPTY          0x40
#define IER_RDA_INT_ENABLE      0x01
#define IER_THRE_INT_ENABLE     0x02
#define IER_SREE_INT_ENABLE     0x20
#define IER_PTIME_INT_ENABLE    0x80
#define IIR_RECV_LINE_ENABLE    0x04
#define IIR_NO_ISQ_PEND         0x01

#define FCR_FIFO_EN             0x01
#define FCR_RX_FIFO_RST         0x02
#define FCR_TX_FIFO_RST         0x04
#define FCR_DMA_MODE            0x08
#define FRC_TX_FIFO_SET(x)      (x << 4)/* 0 is null, 1 is 2byte, 2 is 1/4 fifo, 3 is 1/2 fifo */
#define FRC_RX_FIFO_SET(x)      (x << 6)/* 0 is 1byte, 1 is 1/4 fifo, 2 is 1/2 fifo, 3 is full*/
#define FCR_RX_TRIGGER          0xC0

#define LCR_SET_DLAB            0x80    /* enable r/w DLR to set the baud rate */
#define LCR_PARITY_ENABLE       0x08    /* parity enabled */
#define LCR_PARITY_EVEN         0x10    /* Even parity enabled */
#define LCR_PARITY_ODD          0xef    /* Odd parity enabled */
#define LCR_WORD_SIZE_5         0xfc    /* the data length is 5 bits */
#define LCR_WORD_SIZE_6         0x01    /* the data length is 6 bits */
#define LCR_WORD_SIZE_7         0x02    /* the data length is 7 bits */
#define LCR_WORD_SIZE_8         0x03    /* the data length is 8 bits */
#define LCR_STOP_BIT1           0xfb    /* 1 stop bit */
#define LCR_STOP_BIT2           0x04    /* 1.5 stop bit */

#define HALT_TX_DISABLE         0x00
#define HALT_TX_ENABLE          0x01
#define HALT_CHCFG_AT_BUSY      0x02
#define HALT_CHANGE_UPDATE      0x04

#define AIC_RXCTL_DLAB_ACEN     0X04	/* Receiver divisor latch access bit auto clear enable */
#define AIC_RXCTL_SWITCH        0X01	/* Receiver switch */

#define AIC_LSR_PFE             0x80
#define AIC_LSR_TEMT            0x40
#define AIC_LSR_THRE            0x40
#define AIC_LSR_BI              0x10
#define AIC_LSR_FE              0x08
#define AIC_LSR_PE              0x04
#define AIC_LSR_OE              0x02
#define AIC_LSR_DR              0x01
#define AIC_LSR_TRANS_EMPTY     0x20

#define AIC_IIR_THR_EMPTY       0x02    /* threshold empty */
#define AIC_IIR_RECV_DATA       0x04    /* received data available */
#define AIC_IIR_RECV_LINE       0x06    /* receiver line status */
#define AIC_IIR_CHAR_TIMEOUT    0x0c    /* character timeout */
#define AIC_IIR_SR_EMPTY        0x10    /* shifter reg empty */

/* ArtInChip specific register fields */
#define AIC_UART_MCR_SIRE       0x40
#define AIC_UART_MCR_RS485      0x80
#define AIC_UART_MCR_RS485S     0xC0
#define AIC_UART_MCR_UART       0x00
#define AIC_UART_MCR_FUNC_MASK  0x3F
#define AIC_UART_MCR_FLOW_CTRL  0x22    /* Auto Flow Control Enable */
#define AIC_UART_MCR_RTS_CTRL   0x02    /* RTS Control */
#define AIC_UART_MCR_LOOPBACK   0x10

#define AIC_UART_EXREG          0xB8    /* RS485 DE Time */
#define AIC_UART_RS485_CTL_MODE 0x80
#define AIC_UART_RS485_RXBFA    0x08
#define AIC_UART_RS485_RXAFA    0x04

#define AIC_USART_SET_RS485_NORMAL          0x20            /**< set rs485 normal enable */
#define AIC_USART_SET_RS485_COMPACT_IO      0x21            /**< set rs485 compact-io enable */
#define AIC_USART_CLR_RS485                 0x22            /**< set rs485 disable */

#define AIC_UART_485_CTL_SOFT_MODE0 0x80
#define AIC_UART_485_CTL_SOFT_MODE1 0x81
#define AIC_UART_232_RESUME_DATA    0x83
#define AIC_UART_232_SUSPEND_DATA   0x84
#define AIC_UART_SW_FLOW_CTRL       0x85
#define AIC_UART_SW_RECEIVE_ON_OFF  0x86
#define AIC_UART_SET_BAUDRATE       0x87
#define AIC_UART_ENABLE_SREE_INT    0x89

#define AIC_UART_DEV_MODE_RS485     0x1

#define AIC_UART_IER_RDI        0x1     /* Enable receiver data interrupt */
#define AIC_UART_IER_THRI       0x2     /* Enable Transmitter holding register int. */
#define AIC_UART_IER_RLSI       0x4     /* Enable receiver line status interrupt */
#define AIC_UART_IER_MSI        0x8     /* Enable Modem status interrupt */

#define AIC_UART_TX_DRQ_EN      0x40
#define AIC_UART_RX_DRQ_EN      0x80
#define AIC_UART_FIFO_DEPTH     128
#define AIC_UART_BASE_OFFSET    0x1000
#define AIC_UART_RX_INT         0
#define AIC_UART_TX_INT         1
#define AIC_UART_DMA_HSK_MODE   0xe5
#define AIC_UART_MAX_NUM        8
#define AIC_UART_DMA_MODE(n)    (n << 3)
#define AIC_UART_TX_FIFO_SIZE      2048
#define AIC_UART_RX_FIFO_SIZE      2048
/* DMA_FLAG: RDWR(0x3) | INT_RX(0x100) | DMA_RX(0x200) | DMA_TX(0x800) */
#define AIC_UART_DMA_FLAG       2819
#define AIC_UART_TX_RX_INT_FLAG 1283
/* definition for usart handle. */
typedef void *usart_handle_t;

#define AIC_UART_XOFF               19
#define AIC_UART_XON                17

#define AIC_UART_MAX_BAUDRATE       8000000

/*----- USART Function Codes: -------*/
typedef enum
{
    USART_FUNC_RS232                    = 0,   ///< RS232
    USART_FUNC_RS485                    = 1,   ///< RS485 normal
    USART_FUNC_RS485_COMACT_IO          = 2,   ///< RS485 compact io
    USART_MODE_RS232_AUTO_FLOW_CTRL     = 3,   ///< RS232 flow conctrol
    USART_MODE_RS232_UNAUTO_FLOW_CTRL   = 4,   ///< RS232 hardware unauto flow conctrol
    USART_MODE_RS232_SW_FLOW_CTRL       = 5,   ///< RS232 software flow conctrol
    USART_MODE_RS232_SW_HW_FLOW_CTRL    = 6,   ///< RS232 software and hardware flow conctrol
    USART_FUNC_RS485_SIMULATION         = 7,   ///< RS485 simulation
    USART_FUNC_RS232_SIMULATION         = 8,   ///< RS232 simulation
} usart_func_e;

/****** USART specific error codes *****/
typedef enum
{
    USART_ERROR_MODE  = (DRV_ERROR_SPECIFIC + 1),      ///< Specified Mode not supported
    USART_ERROR_BAUDRATE,                         ///< Specified baudrate not supported
    USART_ERROR_DATA_BITS,                        ///< Specified number of Data bits not supported
    USART_ERROR_PARITY,                           ///< Specified Parity not supported
    USART_ERROR_STOP_BITS,                        ///< Specified number of Stop bits not supported
    USART_ERROR_FLOW_CONTROL,                     ///< Specified Flow Control not supported
    USART_ERROR_CPOL,                             ///< Specified Clock Polarity not supported
    USART_ERROR_CPHA                              ///< Specified Clock Phase not supported
} usart_error_e;

/*----- USART Control Codes: Mode -----*/
typedef enum
{
    USART_MODE_ASYNCHRONOUS         = 0,   ///< USART (Asynchronous)
    USART_MODE_SYNCHRONOUS_MASTER,         ///< Synchronous Master
    USART_MODE_SYNCHRONOUS_SLAVE,          ///< Synchronous Slave (external clock signal)
    USART_MODE_SINGLE_WIRE,                 ///< USART Single-wire (half-duplex)
    USART_MODE_SINGLE_IRDA,                 ///< UART IrDA
    USART_MODE_SINGLE_SMART_CARD,           ///< UART Smart Card
} usart_mode_e;

/*----- USART Control Codes: Mode Parameters: Data Bits -----*/
typedef enum
{
    USART_DATA_BITS_5             = 0,    ///< 5 Data bits
    USART_DATA_BITS_6,                    ///< 6 Data bit
    USART_DATA_BITS_7,                    ///< 7 Data bits
    USART_DATA_BITS_8,                    ///< 8 Data bits (default)
    USART_DATA_BITS_9                     ///< 9 Data bits
} usart_data_bits_e;

/*----- USART Control Codes: Mode Parameters: Parity -----*/
typedef enum
{
    USART_PARITY_NONE            = 0,       ///< No Parity (default)
    USART_PARITY_EVEN,                      ///< Even Parity
    USART_PARITY_ODD,                       ///< Odd Parity
    USART_PARITY_1,                         ///< Parity forced to 1
    USART_PARITY_0                          ///< Parity forced to 0
} usart_parity_e;

/*----- USART Control Codes: Mode Parameters: Stop Bits -----*/
typedef enum
{
    USART_STOP_BITS_1            = 0,    ///< 1 Stop bit (default)
    USART_STOP_BITS_2,                   ///< 2 Stop bits
    USART_STOP_BITS_1_5,                 ///< 1.5 Stop bits
    USART_STOP_BITS_0_5                  ///< 0.5 Stop bits
} usart_stop_bits_e;

/*----- USART Control Codes: Mode Parameters: Clock Polarity (Synchronous mode) -----*/
typedef enum
{
    USART_CPOL0                  = 0,    ///< CPOL = 0 (default). data are captured on rising edge (low->high transition)
    USART_CPOL1                          ///< CPOL = 1. data are captured on falling edge (high->lowh transition)
} usart_cpol_e;

/*----- USART Control Codes: Mode Parameters: Clock Phase (Synchronous mode) -----*/
typedef enum
{
    USART_CPHA0                  = 0,   ///< CPHA = 0 (default). sample on first (leading) edge
    USART_CPHA1                         ///< CPHA = 1. sample on second (trailing) edge
} usart_cpha_e;

/*----- USART Control Codes: flush data type-----*/
typedef enum
{
    USART_FLUSH_WRITE,
    USART_FLUSH_READ
} usart_flush_type_e;

/*----- USART Control Codes: flow control type-----*/
typedef enum
{
    USART_FLOWCTRL_NONE,
    USART_FLOWCTRL_CTS,
    USART_FLOWCTRL_RTS,
    USART_FLOWCTRL_CTS_RTS
} usart_flowctrl_type_e;

/*----- USART Modem Control -----*/
typedef enum
{
    USART_RTS_CLEAR,                  ///< Deactivate RTS
    USART_RTS_SET,                    ///< Activate RTS
    USART_DTR_CLEAR,                  ///< Deactivate DTR
    USART_DTR_SET                     ///< Activate DTR
} usart_modem_ctrl_e;

/*----- USART Modem Status -----*/
typedef struct
{
    uint32_t cts : 1;                     ///< CTS state: 1=Active, 0=Inactive
    uint32_t dsr : 1;                     ///< DSR state: 1=Active, 0=Inactive
    uint32_t dcd : 1;                     ///< DCD state: 1=Active, 0=Inactive
    uint32_t ri  : 1;                     ///< RI  state: 1=Active, 0=Inactive
} usart_modem_stat_t;

/*----- USART Control Codes: on-off intrrupte type-----*/
typedef enum
{
    USART_INTR_WRITE,
    USART_INTR_READ,
    USART_INTR_ELSI,
} usart_intr_type_e;

/**
\brief USART Status
*/
typedef struct  {
    uint32_t tx_busy          : 1;        ///< Transmitter busy flag
    uint32_t rx_busy          : 1;        ///< Receiver busy flag
    uint32_t tx_underflow     : 1;        ///< Transmit data underflow detected (cleared on start of next send operation)(Synchronous Slave)
    uint32_t rx_overflow      : 1;        ///< Receive data overflow detected (cleared on start of next receive operation)
    uint32_t rx_break         : 1;        ///< Break detected on receive (cleared on start of next receive operation)
    uint32_t rx_framing_error : 1;        ///< Framing error detected on receive (cleared on start of next receive operation)
    uint32_t rx_parity_error  : 1;        ///< Parity error detected on receive (cleared on start of next receive operation)
    uint32_t tx_enable        : 1;        ///< Transmitter enable flag
    uint32_t rx_enable        : 1;        ///< Receiver enbale flag
} usart_status_t;

/****** USART Event *****/
typedef enum
{
    USART_EVENT_SEND_COMPLETE       = 0,  ///< Send completed; however USART may still transmit data
    USART_EVENT_RECEIVE_COMPLETE    = 1,  ///< Receive completed
    USART_EVENT_TRANSFER_COMPLETE   = 2,  ///< Transfer completed
    USART_EVENT_TX_COMPLETE         = 3,  ///< Transmit completed (optional)
    USART_EVENT_TX_UNDERFLOW        = 4,  ///< Transmit data not available (Synchronous Slave)
    USART_EVENT_RX_OVERFLOW         = 5,  ///< Receive data overflow
    USART_EVENT_RX_TIMEOUT          = 6,  ///< Receive character timeout (optional)
    USART_EVENT_RX_BREAK            = 7,  ///< Break detected on receive
    USART_EVENT_RX_FRAMING_ERROR    = 8,  ///< Framing error detected on receive
    USART_EVENT_RX_PARITY_ERROR     = 9,  ///< Parity error detected on receive
    USART_EVENT_CTS                 = 10, ///< CTS state changed (optional)
    USART_EVENT_DSR                 = 11, ///< DSR state changed (optional)
    USART_EVENT_DCD                 = 12, ///< DCD state changed (optional)
    USART_EVENT_RI                  = 13, ///< RI  state changed (optional)
    USART_EVENT_RECEIVED            = 14, ///< Data Received, only in usart fifo, call receive()/transfer() get the data
} usart_event_e;

typedef void (*usart_event_cb_t)(int32_t idx, usart_event_e event);   ///< Pointer to \ref usart_event_cb_t : USART Event call back.

/**
\brief USART Driver Capabilities.
*/
typedef struct  {
    uint32_t asynchronous       : 1;      ///< supports UART (Asynchronous) mode
    uint32_t synchronous_master : 1;      ///< supports Synchronous Master mode
    uint32_t synchronous_slave  : 1;      ///< supports Synchronous Slave mode
    uint32_t single_wire        : 1;      ///< supports UART Single-wire mode
    uint32_t irda               : 1;      ///< supports UART IrDA mode
    uint32_t smart_card         : 1;      ///< supports UART Smart Card mode
    uint32_t smart_card_clock   : 1;      ///< Smart Card Clock generator available
    uint32_t flow_control_rts   : 1;      ///< RTS Flow Control available
    uint32_t flow_control_cts   : 1;      ///< CTS Flow Control available
    uint32_t event_tx_complete  : 1;      ///< Transmit completed event: \ref USART_EVENT_TX_COMPLETE
    uint32_t event_rx_timeout   : 1;      ///< Signal receive character timeout event: \ref USART_EVENT_RX_TIMEOUT
    uint32_t rts                : 1;      ///< RTS Line: 0=not available, 1=available
    uint32_t cts                : 1;      ///< CTS Line: 0=not available, 1=available
    uint32_t dtr                : 1;      ///< DTR Line: 0=not available, 1=available
    uint32_t dsr                : 1;      ///< DSR Line: 0=not available, 1=available
    uint32_t dcd                : 1;      ///< DCD Line: 0=not available, 1=available
    uint32_t ri                 : 1;      ///< RI Line: 0=not available, 1=available
    uint32_t event_cts          : 1;      ///< Signal CTS change event: \ref USART_EVENT_CTS
    uint32_t event_dsr          : 1;      ///< Signal DSR change event: \ref USART_EVENT_DSR
    uint32_t event_dcd          : 1;      ///< Signal DCD change event: \ref USART_EVENT_DCD
    uint32_t event_ri           : 1;      ///< Signal RI change event: \ref USART_EVENT_RI
} usart_capabilities_t;

typedef enum
{
    DRV_POWER_OFF,                        ///< Power off: no operation possible
    DRV_POWER_LOW,                        ///< Low Power mode: retain state, detect and signal wake-up events
    DRV_POWER_FULL,                       ///< Power on: full operation at maximum performance
    DRV_POWER_SUSPEND,                    ///< Power suspend: power saving operation
} aic_power_stat_e;


typedef struct aic_usart_priv_t aic_usart_priv_t;
typedef void (*dma_callback)(void *dma_param);
typedef void (*uart_callback)(aic_usart_priv_t *uart, void *arg);

struct aic_uart_dma_transfer_info
{
    struct aic_dma_chan *dma_chan;
    uint8_t *buf;
    uint32_t buf_size;
    uint32_t count;
    void *callback_param;
    dma_callback callback;
};

struct aic_usart_priv_t
{
    size_t base;
    uint32_t irq;
    usart_event_cb_t cb_event;           ///< Event callback
    uint32_t rx_total_num;
    uint32_t tx_total_num;
    uint8_t *rx_buf;
    uint8_t *tx_buf;
    volatile uint32_t rx_cnt;
    volatile uint32_t tx_cnt;
    volatile uint32_t tx_busy;
    volatile uint32_t rx_busy;
    uint32_t last_tx_num;
    uint32_t last_rx_num;
    int32_t idx;
    struct aic_uart_dma_transfer_info dma_tx_info;
    struct aic_uart_dma_transfer_info dma_rx_info;
    uart_callback callback;
    void *arg;
};

struct dma_flag
{
    int8_t dma_enable;
};
/**
  \brief       Initialize USART Interface. 1. Initializes the resources needed for the USART interface 2.registers event callback function
  \param[in]   idx usart index
  \param[in]   cb_event  event call back function \ref usart_event_cb_t
  \return      return usart handle if success
*/
usart_handle_t hal_usart_initialize(int32_t idx, usart_event_cb_t cb_event, void *handler);

/**
  \brief       De-initialize USART Interface. stops operation and releases the software resources used by the interface
  \param[in]   handle  usart handle to operate.
  \return      error code
*/
int32_t hal_usart_uninitialize(usart_handle_t handle);
/**
  \brief       Get driver capabilities.
  \param[in]   idx usart index
  \return      \ref usart_capabilities_t
*/
usart_capabilities_t hal_usart_get_capabilities(int32_t idx);

uint16_t hal_usart_get_irqstatus(int32_t idx);

/**
  \brief       config usart mode.
  \param[in]   handle  usart handle to operate.
  \param[in]   baud      baud rate.
  \param[in]   mode      \ref usart_mode_e .
  \param[in]   parity    \ref usart_parity_e .
  \param[in]   stopbits  \ref usart_stop_bits_e .
  \param[in]   bits      \ref usart_data_bits_e .
  \param[in]   func      \ref usart_func_e .
  \return      error code
*/
int32_t hal_usart_config(usart_handle_t handle,
                         uint32_t baud,
                         usart_mode_e mode,
                         usart_parity_e parity,
                         usart_stop_bits_e stopbits,
                         usart_data_bits_e bits,
                         usart_func_e func);


/**
  \brief       Start sending data to USART transmitter,(received data is ignored).
               This function is non-blocking,\ref usart_event_e is signaled when operation completes or error happens.
               \ref hal_usart_get_status can get operation status.
  \param[in]   handle  usart handle to operate.
  \param[in]   data  Pointer to buffer with data to send to USART transmitter. data_type is : uint8_t for 5..8 data bits, uint16_t for 9 data bits
  \param[in]   num   Number of data items to send
  \return      error code
*/
int32_t hal_usart_send(usart_handle_t handle, const void *data, uint32_t num);

/**
  \brief       Abort Send data to USART transmitter
  \param[in]   handle  usart handle to operate.
  \return      error code
*/
int32_t hal_usart_abort_send(usart_handle_t handle);

/**
  \brief       Start receiving data from USART receiver. \n
               This function is non-blocking,\ref usart_event_e is signaled when operation completes or error happens.
               \ref hal_usart_get_status can get operation status.
  \param[in]   handle  usart handle to operate.
  \param[out]  data  Pointer to buffer for data to receive from USART receiver.data_type is : uint8_t for 5..8 data bits, uint16_t for 9 data bits
  \param[in]   num   Number of data items to receive
  \return      error code
*/
int32_t hal_usart_receive(usart_handle_t handle, void *data, uint32_t num);

/**
  \brief       query data from UART receiver FIFO.
  \param[in]   handle  usart handle to operate.
  \param[out]  data  Pointer to buffer for data to receive from UART receiver
  \param[in]   num   Number of data items to receive
  \return      fifo data num to receive
*/
int32_t hal_usart_receive_query(usart_handle_t handle, void *data, uint32_t num);

/**
  \brief       Abort Receive data from USART receiver
  \param[in]   handle  usart handle to operate.
  \return      error code
*/
int32_t hal_usart_abort_receive(usart_handle_t handle);

/**
  \brief       Start synchronously sends data to the USART transmitter and receives data from the USART receiver. used in synchronous mode
               This function is non-blocking,\ref usart_event_e is signaled when operation completes or error happens.
               \ref hal_usart_get_status can get operation status.
  \param[in]   handle  usart handle to operate.
  \param[in]   data_out  Pointer to buffer with data to send to USART transmitter.data_type is : uint8_t for 5..8 data bits, uint16_t for 9 data bits
  \param[out]  data_in   Pointer to buffer for data to receive from USART receiver.data_type is : uint8_t for 5..8 data bits, uint16_t for 9 data bits
  \param[in]   num       Number of data items to transfer
  \return      error code
*/
int32_t hal_usart_transfer(usart_handle_t handle, const void *data_out, void *data_in, uint32_t num);

/**
  \brief       abort sending/receiving data to/from USART transmitter/receiver.
  \param[in]   handle  usart handle to operate.
  \return      error code
*/
int32_t hal_usart_abort_transfer(usart_handle_t handle);

/**
  \brief       Get USART status.
  \param[in]   handle  usart handle to operate.
  \return      USART status \ref usart_status_t
*/
usart_status_t hal_usart_get_status(usart_handle_t handle);

/**
  \brief       flush receive/send data.
  \param[in]   handle usart handle to operate.
  \param[in]   type \ref usart_flush_type_e .
  \return      error code
*/
int32_t hal_usart_flush(usart_handle_t handle, usart_flush_type_e type);

/**
  \brief       set interrupt mode.
  \param[in]   handle usart handle to operate.
  \param[in]   type \ref usart_intr_type_e.
  \param[in]   flag 0-OFF, 1-ON.
  \return      error code
*/
int32_t hal_usart_set_interrupt(usart_handle_t handle, usart_intr_type_e type, int32_t flag);

/**
  \brief       set the baud rate of usart.
  \param[in]   baud  usart base to operate.
  \param[in]   baudrate baud rate
  \return      error code
*/
int32_t hal_usart_config_baudrate(usart_handle_t handle, uint32_t baud);

/**
  \brief       config usart mode.
  \param[in]   handle  usart handle to operate.
  \param[in]   mode    \ref usart_mode_e
  \return      error code
*/
int32_t hal_usart_config_mode(usart_handle_t handle, usart_mode_e mode);

/**
  \brief       config usart parity.
  \param[in]   handle  usart handle to operate.
  \param[in]   parity    \ref usart_parity_e
  \return      error code
*/
int32_t hal_usart_config_parity(usart_handle_t handle, usart_parity_e parity);

/**
  \brief       config usart stop bit number.
  \param[in]   handle  usart handle to operate.
  \param[in]   stopbits  \ref usart_stop_bits_e
  \return      error code
*/
int32_t hal_usart_config_stopbits(usart_handle_t handle, usart_stop_bits_e stopbit);

/**
  \brief       config usart data length.
  \param[in]   handle  usart handle to operate.
  \param[in]   databits      \ref usart_data_bits_e
  \return      error code
*/
int32_t hal_usart_config_databits(usart_handle_t handle, usart_data_bits_e databits);

/**
  \brief       get character in query mode.
  \param[in]   handle  usart handle to operate.
  \param[out]  ch the pointer to the received character.
  \return      error code
*/
int32_t hal_usart_getchar(usart_handle_t handle, uint8_t *ch);

int hal_uart_getchar(usart_handle_t handle);

int32_t hal_usart_clr_int_flag(usart_handle_t handle,uint32_t flag);
int32_t hal_usart_set_int_flag(usart_handle_t handle,uint32_t flag);

/**
  \brief       transmit character in query mode.
  \param[in]   handle  usart handle to operate.
  \param[in]   ch  the input character
  \return      error code
*/
int32_t hal_usart_putchar(usart_handle_t handle, uint8_t ch);

/**
  \brief       Get usart send data count.
  \param[in]   handle  usart handle to operate.
  \return      number of currently transmitted data bytes
*/
uint32_t hal_usart_get_tx_count(usart_handle_t handle);

/**
  \brief       Get usart received data count.
  \param[in]   handle  usart handle to operate.
  \return      number of currently received data bytes
*/
uint32_t hal_usart_get_rx_count(usart_handle_t handle);

/**
  \brief       control usart power.
  \param[in]   handle  usart handle to operate.
  \param[in]   state   power state.\ref aic_power_stat_e.
  \return      error code
*/
int32_t hal_usart_power_control(usart_handle_t handle, aic_power_stat_e state);

/**
  \brief       config usart flow control type.
  \param[in]   handle  usart handle to operate.
  \param[in]   flowctrl_type   flow control type.\ref usart_flowctrl_type_e.
  \return      error code
*/
int32_t hal_usart_config_flowctrl(usart_handle_t handle,
                                  usart_flowctrl_type_e flowctrl_type);

/**
  \brief       config usart clock Polarity and Phase.
  \param[in]   handle  usart handle to operate.
  \param[in]   cpol    Clock Polarity.\ref usart_cpol_e.
  \param[in]   cpha    Clock Phase.\ref usart_cpha_e.
  \return      error code
*/
int32_t hal_usart_config_clock(usart_handle_t handle, usart_cpol_e cpol, usart_cpha_e cpha);

/**
  \brief       control the transmitter.
  \param[in]   handle  usart handle to operate.
  \param[in]   enable  1 - enable the transmitter. 0 - disable the transmitter
  \return      error code
*/
int32_t hal_usart_control_tx(usart_handle_t handle, uint32_t enable);

/**
  \brief       control the receiver.
  \param[in]   handle  usart handle to operate.
  \param[in]   enable  1 - enable the receiver. 0 - disable the receiver
  \return      error code
*/
int32_t hal_usart_control_rx(usart_handle_t handle, uint32_t enable);

/**
  \brief       control the break.
  \param[in]   handle  usart handle to operate.
  \param[in]   enable  1- Enable continuous Break transmission,0 - disable continuous Break transmission
  \return      error code
*/
int32_t hal_usart_control_break(usart_handle_t handle, uint32_t enable);

/**
  \brief       config usart to 485 mode.
  \param[in]   handle       usart handle to operate.
  \param[in]   rs485        enable rs485
  \param[in]   compactio    compactio
  \return      error code
*/
int32_t hal_uart_reset_fifo(usart_handle_t handle);
int32_t hal_usart_set_loopback(usart_handle_t handle, uint8_t enable);
int32_t hal_usart_config_rs485(usart_handle_t handle, bool rs485, bool compactio);
int32_t hal_usart_rts_ctl_soft_mode_set(usart_handle_t handle);
int32_t hal_usart_rts_ctl_soft_mode_clr(usart_handle_t handle);
int32_t hal_usart_halt_tx_enable(usart_handle_t handle, uint8_t halt_tx_enable);
uint32_t hal_usart_get_cur_baudrate(usart_handle_t handle);
void hal_usart_intr_recv_line(int32_t idx, aic_usart_priv_t *usart_priv);
void hal_usart_clear_rxfifo(usart_handle_t handle);
int32_t hal_uart_set_sree_int(usart_handle_t handle, int32_t flag);

#if defined(RT_SERIAL_USING_DMA) || defined(AIC_SERIAL_USING_DMA)
int32_t hal_uart_set_fifo(usart_handle_t handle);
int32_t hal_usart_tx_enable_drq(usart_handle_t handle);
int32_t hal_usart_rx_enable_drq(usart_handle_t handle);
int32_t hal_usart_tx_disable_drq(usart_handle_t handle);
int32_t hal_usart_rx_disable_drq(usart_handle_t handle);
int32_t hal_usart_get_rx_fifo_num(usart_handle_t handle);
int32_t hal_usart_set_hsk(usart_handle_t handle);
int32_t hal_uart_rx_dma_config(usart_handle_t handle, uint8_t *buf, uint32_t size);
int32_t hal_uart_send_by_dma(usart_handle_t handle, uint8_t *buf, uint32_t size);
int32_t hal_uart_attach_callback(aic_usart_priv_t *uart, uart_callback callback,
                                void *arg);
int32_t hal_uart_detach_callback(aic_usart_priv_t *uart);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _AIC_HAL_UART_ */
