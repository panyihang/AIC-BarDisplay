/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_drv.h>
#include "aic_hal_uart.h"
#include "aic_drv_uart.h"

#ifndef AIC_CLK_UART0_FREQ
#define AIC_CLK_UART0_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART1_FREQ
#define AIC_CLK_UART1_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART2_FREQ
#define AIC_CLK_UART2_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART3_FREQ
#define AIC_CLK_UART3_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART4_FREQ
#define AIC_CLK_UART4_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART5_FREQ
#define AIC_CLK_UART5_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART6_FREQ
#define AIC_CLK_UART6_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART7_FREQ
#define AIC_CLK_UART7_FREQ 48000000 /* default 48M*/
#endif

#ifndef AIC_UART0_CTS_NAME
#define AIC_UART0_CTS_NAME "no_pin"
#endif
#ifndef AIC_UART1_CTS_NAME
#define AIC_UART1_CTS_NAME "no_pin"
#endif
#ifndef AIC_UART2_CTS_NAME
#define AIC_UART2_CTS_NAME "no_pin"
#endif
#ifndef AIC_UART3_CTS_NAME
#define AIC_UART3_CTS_NAME "no_pin"
#endif
#ifndef AIC_UART4_CTS_NAME
#define AIC_UART4_CTS_NAME "no_pin"
#endif
#ifndef AIC_UART5_CTS_NAME
#define AIC_UART5_CTS_NAME "no_pin"
#endif
#ifndef AIC_UART6_CTS_NAME
#define AIC_UART6_CTS_NAME "no_pin"
#endif
#ifndef AIC_UART7_CTS_NAME
#define AIC_UART7_CTS_NAME "no_pin"
#endif

#ifndef AIC_UART0_RTS_NAME
#define AIC_UART0_RTS_NAME "no_pin"
#endif
#ifndef AIC_UART1_RTS_NAME
#define AIC_UART1_RTS_NAME "no_pin"
#endif
#ifndef AIC_UART2_RTS_NAME
#define AIC_UART2_RTS_NAME "no_pin"
#endif
#ifndef AIC_UART3_RTS_NAME
#define AIC_UART3_RTS_NAME "no_pin"
#endif
#ifndef AIC_UART4_RTS_NAME
#define AIC_UART4_RTS_NAME "no_pin"
#endif
#ifndef AIC_UART5_RTS_NAME
#define AIC_UART5_RTS_NAME "no_pin"
#endif
#ifndef AIC_UART6_RTS_NAME
#define AIC_UART6_RTS_NAME "no_pin"
#endif
#ifndef AIC_UART7_RTS_NAME
#define AIC_UART7_RTS_NAME "no_pin"
#endif

#ifndef AIC_UART0_RS232_CTRL_NAME
#define AIC_UART0_RS232_CTRL_NAME "no_pin"
#endif
#ifndef AIC_UART1_RS232_CTRL_NAME
#define AIC_UART1_RS232_CTRL_NAME "no_pin"
#endif
#ifndef AIC_UART2_RS232_CTRL_NAME
#define AIC_UART2_RS232_CTRL_NAME "no_pin"
#endif
#ifndef AIC_UART3_RS232_CTRL_NAME
#define AIC_UART3_RS232_CTRL_NAME "no_pin"
#endif
#ifndef AIC_UART4_RS232_CTRL_NAME
#define AIC_UART4_RS232_CTRL_NAME "no_pin"
#endif
#ifndef AIC_UART5_RS232_CTRL_NAME
#define AIC_UART5_RS232_CTRL_NAME "no_pin"
#endif
#ifndef AIC_UART6_RS232_CTRL_NAME
#define AIC_UART6_RS232_CTRL_NAME "no_pin"
#endif
#ifndef AIC_UART7_RS232_CTRL_NAME
#define AIC_UART7_RS232_CTRL_NAME "no_pin"
#endif

struct aic_uart_rts_cts_dev
{
    unsigned int uart_rts_pin;
    unsigned int uart_cts_pin;
    unsigned int uart_ctrl_pin;
    usart_handle_t handle;
};

volatile struct aic_uart_rts_cts_dev uart_rts_dev[AIC_UART_DEV_NUM];
volatile struct aic_uart_rts_cts_dev uart_cts_dev[AIC_UART_DEV_NUM];
volatile struct aic_uart_rts_cts_dev uart_ctrl_dev[AIC_UART_DEV_NUM];

void drv_usart_irqhandler(int irq, void * data);
void drv_usart_set_freq(uint32_t baudrate, int u);
static rt_err_t drv_uart_control(struct rt_serial_device *serial, int cmd, void *arg);

struct
{
    uint32_t base;
    uint32_t irq;
    void *handler;
}
const drv_usart_config[AIC_UART_DEV_NUM] =
{
#if (AIC_UART_DEV_NUM >= 4)
    {UART_BASE(0), UART_IRQn(0), drv_usart_irqhandler},
    {UART_BASE(1), UART_IRQn(1), drv_usart_irqhandler},
    {UART_BASE(2), UART_IRQn(2), drv_usart_irqhandler},
    {UART_BASE(3), UART_IRQn(3), drv_usart_irqhandler},
#endif
#if (AIC_UART_DEV_NUM >= 8)
    {UART_BASE(4), UART_IRQn(4), drv_usart_irqhandler},
    {UART_BASE(5), UART_IRQn(5), drv_usart_irqhandler},
    {UART_BASE(6), UART_IRQn(6), drv_usart_irqhandler},
    {UART_BASE(7), UART_IRQn(7), drv_usart_irqhandler},
#endif
};

static volatile usart_handle_t uart_handle[AIC_UART_DEV_NUM];
static volatile struct rt_serial_device g_serial[AIC_UART_DEV_NUM];

#if defined (AIC_SERIAL_USING_DMA)
#define UART_ALIGN_SIZE     CACHE_LINE_SIZE
struct aic_uart_info {
    uint32_t *uart_tx_fifo;
    uint32_t *uart_rx_fifo;
    uint32_t rx_size;
    rt_sem_t tx_semaphore;
};
static volatile struct aic_uart_info g_info[AIC_UART_DEV_NUM] = {0};

static char uart_sem_name[][9] = {
    "uart_tx0",
    "uart_tx1",
    "uart_tx2",
    "uart_tx3",
    "uart_tx4",
    "uart_tx5",
    "uart_tx6",
    "uart_tx7",
};

#endif

struct uart_freq_baud
{
    uint32_t  baud;
    uint32_t  freq;
}uart_freq_baud;

struct uart_freq_baud uart_freq_baud_list[] =
{
#if defined(AIC_CHIP_D12X) // PLL 588 MHz
    {300, 53454545},
    {600, 53454545},
    {1200, 53454545},
    {2400, 53454545},
    {4800, 53454545},
    {9600, 53454545},
    {14400, 53454545},
    {19200, 53454545},
    {38400, 53454545},
    {57600, 53454545},
    {115200, 53454545},
    {230400, 58800000},
    {380400, 49000000},
    {460800, 58800000},
    {921600, 58800000},
    {1000000, 49000000},
    {1152000, 36750000},
    {1500000, 49000000},
    {1750000, 84000000},
    {2000000, 65333333},
    {2450000, 39200000},
    {2500000, 39200000},
    {3000000, 49000000},
#else // PLL 1200 MHz
    {300, 48000000},
    {600, 48000000},
    {1200, 48000000},
    {2400, 48000000},
    {4800, 48000000},
    {9600, 48000000},
    {14400, 48000000},
    {19200, 48000000},
    {38400, 48000000},
    {57600, 48000000},
    {115200, 48000000},
    {230400, 48000000},
    {380400, 54545454},
    {460800, 44444444},
    {921600, 44444444},
    {1000000, 48000000},
    {1152000, 54545454},
    {1500000, 48000000},
    {2500000, 40000000},
    {3000000, 48000000},
#endif
};

int32_t drv_usart_target_init(int32_t idx, uint32_t *base, uint32_t *irq, void **handler)
{
    if (idx >= AIC_UART_DEV_NUM)
    {
        return -1;
    }

    if (base != NULL)
    {
        *base = drv_usart_config[idx].base;
    }

    if (irq != NULL)
    {
        *irq = drv_usart_config[idx].irq;
    }

    if (handler != NULL)
    {
        *handler = drv_usart_config[idx].handler;
    }

    return idx;
}

void drv_usart_cb_event(int32_t index, usart_event_e event)
{
    usart_handle_t uart;
    RT_ASSERT(g_serial != RT_NULL);
    uart = (usart_handle_t)g_serial[index].parent.user_data;

    switch (event)
    {
    case USART_EVENT_RX_BREAK:
    case USART_EVENT_RX_FRAMING_ERROR:
    case USART_EVENT_RX_PARITY_ERROR:
        hal_usart_clear_rxfifo(uart);
        break;
    default:
        break;
    }
    return;
}

void drv_usart_irqhandler(int irq, void * data)
{
    int index = irq - UART_IRQn(0);
    uint16_t status= 0;
    usart_handle_t uart;

    if (index < 0 || index >= AIC_UART_DEV_NUM)
        return;

    RT_ASSERT(g_serial != RT_NULL);
    uart = (usart_handle_t)g_serial[index].parent.user_data;

    status = hal_usart_get_irqstatus(index);

    if (g_serial[index].config.function != USART_FUNC_RS232_SIMULATION &&
        g_serial[index].config.function != USART_FUNC_RS485_SIMULATION)
        status &= 0xf;

    if (g_serial[index].config.flag == AIC_UART_DMA_FLAG) {
#if defined (AIC_SERIAL_USING_DMA)
        RT_ASSERT(uart != RT_NULL);
        hal_usart_set_interrupt(uart, USART_INTR_READ, 0);
        switch (status & 0x0f)
        {
        case AIC_IIR_RECV_DATA:
        case AIC_IIR_CHAR_TIMEOUT:
            g_info[index].rx_size = hal_usart_get_rx_fifo_num(uart);
            hal_uart_rx_dma_config(uart, (uint8_t *)g_info[index].uart_rx_fifo, g_info[index].rx_size);
            break;

        case AIC_IIR_RECV_LINE:
            hal_usart_intr_recv_line(index, (aic_usart_priv_t *)g_serial[index].parent.user_data);
            hal_usart_set_interrupt(uart, USART_INTR_READ, 1);
            break;

        default:
            break;
        }
#endif
    } else {
        if (AIC_IIR_SR_EMPTY & status) {
            hal_uart_set_sree_int(uart, 0);
            if (g_serial[index].config.function == USART_FUNC_RS232_SIMULATION) {
                if (uart_ctrl_dev[index].uart_ctrl_pin)
                    rt_pin_write(uart_ctrl_dev[index].uart_ctrl_pin, PIN_LOW);
            } else if (g_serial[index].config.function == USART_FUNC_RS485_SIMULATION) {
                hal_usart_rts_ctl_soft_mode_clr(uart);
                rt_pin_write(uart_rts_dev[index].uart_rts_pin, PIN_LOW);
            }
        }

        switch (status & 0x0f) {
        case AIC_IIR_RECV_DATA:
        case AIC_IIR_CHAR_TIMEOUT:
        {
            struct rt_serial_rx_fifo *rx_fifo = (struct rt_serial_rx_fifo *)g_serial[index].serial_rx;
            int rx_len = 0;

            if (rx_fifo) {
                rx_len = (rx_fifo->put_index >= rx_fifo->get_index) ?
                         (rx_fifo->put_index - rx_fifo->get_index) :
                         (g_serial[index].config.bufsz - (rx_fifo->get_index - rx_fifo->put_index));
                if (rx_fifo->is_full) {
                    rx_len = g_serial[index].config.bufsz;
                }
            }

            /* Auto flow control: if external ringbuf is almost full, disable RX
             * interrupt and let data stay in hardware FIFO. Hardware will
             * auto-assert RTS to stop the peer from sending more data.
             */
            if (g_serial[index].config.function == USART_MODE_RS232_AUTO_FLOW_CTRL &&
                rx_len >= (g_serial[index].config.bufsz - 16)) {
                hal_usart_set_interrupt(uart, USART_INTR_READ, 0);
                break;
            }

            rt_hw_serial_isr((struct rt_serial_device *)&g_serial[index], RT_SERIAL_EVENT_RX_IND);
            break;
        }
        case AIC_IIR_RECV_LINE:
            hal_usart_set_interrupt(uart, USART_INTR_READ, 0);
            hal_usart_intr_recv_line(index, (aic_usart_priv_t *)g_serial[index].parent.user_data);
            hal_usart_set_interrupt(uart, USART_INTR_READ, 1);
            break;
        default:
            break;
        }
    }
}

/*
 * UART interface
 */
static rt_err_t drv_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    int ret;
    uint32_t index = 0;
    usart_handle_t uart;

    uint32_t bauds;
    usart_mode_e mode;
    usart_parity_e parity;
    usart_stop_bits_e stopbits;
    usart_data_bits_e databits;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    /* set baudrate parity...*/
    index = serial->config.uart_index;
    if (index >= AIC_UART_DEV_NUM) {
        hal_log_err("index parameter error(%d)!\n", index);
        return -RT_EINVAL;
    }

    bauds = cfg->baud_rate;
    if (bauds >= AIC_UART_MAX_BAUDRATE) {
        hal_log_err("bauds parameter error(%d)!\n", bauds);
        return -RT_EINVAL;
    }

    mode = USART_MODE_ASYNCHRONOUS;

    if (cfg->parity == PARITY_EVEN)
        parity = USART_PARITY_EVEN;
    else if (cfg->parity == PARITY_ODD)
        parity = USART_PARITY_ODD;
    else
        parity = USART_PARITY_NONE;

    if (cfg->stop_bits == STOP_BITS_1)
        stopbits = USART_STOP_BITS_1;
    else if (cfg->stop_bits == STOP_BITS_2)
        stopbits = USART_STOP_BITS_2;
    else if (cfg->stop_bits == STOP_BITS_3)
        stopbits = USART_STOP_BITS_1_5;
    else
        stopbits = USART_STOP_BITS_0_5;

    if (cfg->data_bits == 5)
        databits = USART_DATA_BITS_5;
    else if (cfg->data_bits == 6)
        databits = USART_DATA_BITS_6;
    else if (cfg->data_bits == 7)
        databits = USART_DATA_BITS_7;
    else if (cfg->data_bits == 8)
        databits = USART_DATA_BITS_8;
    else if (cfg->data_bits == 9)
        databits = USART_DATA_BITS_9;
    else
        databits = USART_DATA_BITS_8;

    hal_clk_disable(CLK_UART0 + index);
    drv_usart_set_freq(bauds, index);
    hal_clk_enable(CLK_UART0 + index);
    aic_udelay(1000);
    hal_usart_set_loopback(uart, 1);
    hal_uart_reset_fifo(uart);

    ret = hal_usart_config(uart, bauds, mode, parity, stopbits, databits, cfg->function);
    if (ret < 0)
    {
        hal_usart_set_loopback(uart, 0);
        return -RT_ERROR;
    }

    hal_usart_set_loopback(uart, 0);

    return RT_EOK;
}

rt_err_t aic_uart_configure(u32 index, struct serial_configure *cfg)
{
    if (index >= AIC_UART_DEV_NUM) {
        pr_err("Invalid UART port NO. %d\n", index);
        return RT_EINVAL;
    }

    if (!cfg) {
        pr_err("Invalid serial_configure\n");
        return RT_EINVAL;
    }

    return drv_uart_configure((struct rt_serial_device *)&g_serial[index], cfg);
}

static rt_err_t drv_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    usart_handle_t uart;
    aic_usart_priv_t *uart_data;
    char off = AIC_UART_XOFF;
    char on = AIC_UART_XON;
    u32 index = 0;


    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    uart_data = serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* Disable the UART Interrupt */
        if ((uintptr_t)arg == RT_DEVICE_FLAG_INT_RX)
            hal_usart_set_interrupt(uart, USART_INTR_READ, 0);
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* Enable the UART Interrupt */
        if ((uintptr_t)arg == RT_DEVICE_FLAG_INT_RX)
            hal_usart_set_interrupt(uart, USART_INTR_READ, 1);
        break;

    case AIC_UART_485_CTL_SOFT_MODE0:
        hal_usart_rts_ctl_soft_mode_clr(uart);
        rt_pin_write(uart_rts_dev[uart_data->idx].uart_rts_pin, PIN_LOW);
        break;

    case AIC_UART_485_CTL_SOFT_MODE1:
        hal_usart_rts_ctl_soft_mode_set(uart);
        rt_pin_write(uart_rts_dev[uart_data->idx].uart_rts_pin, PIN_HIGH);
        break;

     case AIC_UART_232_RESUME_DATA:
        if (serial->config.function == USART_MODE_RS232_UNAUTO_FLOW_CTRL ||
            serial->config.function == USART_MODE_RS232_SW_HW_FLOW_CTRL) {
            rt_pin_write(uart_rts_dev[uart_data->idx].uart_rts_pin, PIN_LOW);
        }
        if (serial->config.function == USART_MODE_RS232_SW_FLOW_CTRL ||
            serial->config.function == USART_MODE_RS232_SW_HW_FLOW_CTRL) {
        #ifdef RT_USING_DEVICE_OPS
            serial->parent.ops->write(&serial->parent, 0,  &on, 1);
        #else
            serial->parent.write(&serial->parent, 0,  &on, 1);
        #endif
        }
        break;

    case AIC_UART_232_SUSPEND_DATA:
        if (serial->config.function == USART_MODE_RS232_UNAUTO_FLOW_CTRL ||
            serial->config.function == USART_MODE_RS232_SW_HW_FLOW_CTRL) {
            rt_pin_write(uart_rts_dev[uart_data->idx].uart_rts_pin, PIN_HIGH);
        }
        if (serial->config.function == USART_MODE_RS232_SW_FLOW_CTRL ||
            serial->config.function == USART_MODE_RS232_SW_HW_FLOW_CTRL) {
        #ifdef RT_USING_DEVICE_OPS
            serial->parent.ops->write(&serial->parent, 0,  &off, 1);
        #else
            serial->parent.write(&serial->parent, 0,  &off, 1);
        #endif
        }
        break;

    case AIC_UART_SW_RECEIVE_ON_OFF:
        if (*(char *)arg == AIC_UART_XOFF) {
            hal_usart_halt_tx_enable(uart, HALT_TX_ENABLE);
        } else if (*(char *)arg == AIC_UART_XON) {
            hal_usart_halt_tx_enable(uart, HALT_TX_DISABLE);
        }
        break;

    case AIC_UART_SET_BAUDRATE:
        hal_clk_disable(CLK_UART0 + serial->config.uart_index);
        drv_usart_set_freq(*(uint32_t *)arg, serial->config.uart_index);
        hal_clk_enable(CLK_UART0 + serial->config.uart_index);
        aic_udelay(1000);
        hal_usart_set_loopback(uart, 1);
        hal_uart_reset_fifo(uart);
        hal_usart_config_baudrate(uart, *(uint32_t *)arg);
        hal_usart_set_loopback(uart, 0);
        break;

    case AIC_UART_ENABLE_SREE_INT:
        index = serial->config.uart_index;
        if (g_serial[index].config.function == USART_FUNC_RS232_SIMULATION) {
            hal_uart_set_sree_int(uart, 1);
            if (uart_ctrl_dev[index].uart_ctrl_pin)
                rt_pin_write(uart_ctrl_dev[index].uart_ctrl_pin, PIN_HIGH);
        } else if (g_serial[index].config.function == USART_FUNC_RS485_SIMULATION) {
            hal_uart_set_sree_int(uart, 1);
            hal_usart_rts_ctl_soft_mode_set(uart);
            rt_pin_write(uart_rts_dev[uart_data->idx].uart_rts_pin, PIN_HIGH);
        }
        break;
    }

    return (RT_EOK);
}

static int drv_uart_putc(struct rt_serial_device *serial, char c)
{
    usart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);
    hal_usart_putchar(uart, c);

    return (1);
}

static int drv_uart_getc(struct rt_serial_device *serial)
{
    int ch;
    usart_handle_t uart;
    struct rt_serial_rx_fifo *rx_fifo;
    int rx_len = 0;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    ch = hal_uart_getchar(uart);

    /* Auto flow control: re-enable RX interrupt once the external ringbuf
     * has enough free space again, so that buffered data in hardware FIFO
     * can be drained.
     */
    if (serial->config.function == USART_MODE_RS232_AUTO_FLOW_CTRL) {
        rx_fifo = (struct rt_serial_rx_fifo *)serial->serial_rx;
        if (rx_fifo) {
            rx_len = (rx_fifo->put_index >= rx_fifo->get_index) ?
                     (rx_fifo->put_index - rx_fifo->get_index) :
                     (serial->config.bufsz - (rx_fifo->get_index - rx_fifo->put_index));
            if (rx_fifo->is_full) {
                rx_len = serial->config.bufsz;
            }
        }

        if (rx_len < (serial->config.bufsz - 32)) {
            hal_usart_set_interrupt(uart, USART_INTR_READ, 1);
        }
    }

    return ch;
}

#if defined (AIC_SERIAL_USING_DMA)
#include <string.h>
static void drv_uart_callback(aic_usart_priv_t *uart, void *arg)
{
    unsigned long event = (unsigned long)arg;
    struct rt_serial_rx_fifo *rx_fifo;

    switch(event)
    {
    case AIC_UART_TX_INT:
        rt_hw_serial_isr((struct rt_serial_device *)&g_serial[uart->idx], RT_SERIAL_EVENT_TX_DMADONE);
        rt_sem_release(g_info[uart->idx].tx_semaphore);
        break;

    case AIC_UART_RX_INT:
        rx_fifo = (struct rt_serial_rx_fifo *)g_serial[uart->idx].serial_rx;
        aicos_dcache_invalid_range((void *)g_info[uart->idx].uart_rx_fifo, AIC_UART_RX_FIFO_SIZE);
        if (rx_fifo->put_index + g_info[uart->idx].rx_size < g_serial[uart->idx].config.bufsz) {
            memcpy((rx_fifo->buffer + rx_fifo->put_index), (rt_uint8_t *)g_info[uart->idx].uart_rx_fifo, g_info[uart->idx].rx_size);
        } else {
            memcpy((rx_fifo->buffer + rx_fifo->put_index), (rt_uint8_t *)g_info[uart->idx].uart_rx_fifo,
                    g_serial[uart->idx].config.bufsz - rx_fifo->put_index);
            memcpy((rx_fifo->buffer), ((rt_uint8_t *)g_info[uart->idx].uart_rx_fifo + g_serial[uart->idx].config.bufsz -
                    rx_fifo->put_index), g_info[uart->idx].rx_size + rx_fifo->put_index - g_serial[uart->idx].config.bufsz);
        }
        hal_usart_set_interrupt(uart, USART_INTR_READ, 1);

        rt_hw_serial_isr((struct rt_serial_device *)&g_serial[uart->idx], RT_SERIAL_EVENT_RX_DMADONE | (g_info[uart->idx].rx_size << 8));
        break;

    default:
        hal_log_err("not support event\n");
        break;
    }
}

static rt_size_t drv_uart_dma_transmit(struct rt_serial_device *serial, rt_uint8_t *buf,
                                        rt_size_t size, int direction)
{
    usart_handle_t uart;
    rt_uint8_t index;
    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    index = serial->config.uart_index;

    if (direction == RT_SERIAL_DMA_TX) {
        if (size > AIC_UART_TX_FIFO_SIZE)
            return 0;
        memcpy((rt_uint8_t *)g_info[index].uart_tx_fifo, buf, size);
        aicos_dcache_clean_range((void *)g_info[index].uart_tx_fifo, AIC_UART_TX_FIFO_SIZE);
        if (hal_uart_send_by_dma(uart, (rt_uint8_t *)g_info[index].uart_tx_fifo, size) == 0) {
            rt_sem_take(g_info[index].tx_semaphore, RT_WAITING_FOREVER);
            return size;
        }
    }
    return 0;
}
#endif

const struct rt_uart_ops drv_uart_ops =
{
    drv_uart_configure,
    drv_uart_control,
    drv_uart_putc,
    drv_uart_getc,
#if defined (AIC_SERIAL_USING_DMA)
    drv_uart_dma_transmit,
#endif
};

struct drv_uart_dev_para
{
    uint32_t index                   :4;
    uint32_t data_bits               :4;
    uint32_t stop_bits               :2;
    uint32_t parity                  :2;
    uint32_t flag;
    uint32_t baud_rate;
    uint32_t clk_freq;
    uint32_t function;
    char * name;
    char * uart_rts_name;
    char * uart_cts_name;
    char * rs232_ctrl_name;
};

static const struct drv_uart_dev_para uart_dev_paras[] =
{
#ifdef AIC_USING_UART0
    {0, AIC_DEV_UART0_DATABITS, AIC_DEV_UART0_STOPBITS, AIC_DEV_UART0_PARITY, AIC_UART0_FLAG,
     AIC_DEV_UART0_BAUDRATE, AIC_CLK_UART0_FREQ, AIC_DEV_UART0_MODE, "uart0", AIC_UART0_RTS_NAME,
    AIC_UART0_CTS_NAME, AIC_UART0_RS232_CTRL_NAME},
#endif
#ifdef AIC_USING_UART1
    {1, AIC_DEV_UART1_DATABITS, AIC_DEV_UART1_STOPBITS, AIC_DEV_UART1_PARITY, AIC_UART1_FLAG,
     AIC_DEV_UART1_BAUDRATE, AIC_CLK_UART1_FREQ, AIC_DEV_UART1_MODE, "uart1", AIC_UART1_RTS_NAME,
     AIC_UART1_CTS_NAME, AIC_UART1_RS232_CTRL_NAME},
#endif
#ifdef AIC_USING_UART2
    {2, AIC_DEV_UART2_DATABITS, AIC_DEV_UART2_STOPBITS, AIC_DEV_UART2_PARITY, AIC_UART2_FLAG,
     AIC_DEV_UART2_BAUDRATE, AIC_CLK_UART2_FREQ, AIC_DEV_UART2_MODE, "uart2", AIC_UART2_RTS_NAME,
     AIC_UART2_CTS_NAME, AIC_UART2_RS232_CTRL_NAME},
#endif
#ifdef AIC_USING_UART3
    {3, AIC_DEV_UART3_DATABITS, AIC_DEV_UART3_STOPBITS, AIC_DEV_UART3_PARITY, AIC_UART3_FLAG,
     AIC_DEV_UART3_BAUDRATE, AIC_CLK_UART3_FREQ, AIC_DEV_UART3_MODE, "uart3", AIC_UART3_RTS_NAME,
     AIC_UART3_CTS_NAME, AIC_UART3_RS232_CTRL_NAME},
#endif
#ifdef AIC_USING_UART4
    {4, AIC_DEV_UART4_DATABITS, AIC_DEV_UART4_STOPBITS, AIC_DEV_UART4_PARITY, AIC_UART4_FLAG,
     AIC_DEV_UART4_BAUDRATE, AIC_CLK_UART4_FREQ, AIC_DEV_UART4_MODE, "uart4", AIC_UART4_RTS_NAME,
     AIC_UART4_CTS_NAME, AIC_UART4_RS232_CTRL_NAME},
#endif
#ifdef AIC_USING_UART5
    {5, AIC_DEV_UART5_DATABITS, AIC_DEV_UART5_STOPBITS, AIC_DEV_UART5_PARITY, AIC_UART5_FLAG,
     AIC_DEV_UART5_BAUDRATE, AIC_CLK_UART5_FREQ, AIC_DEV_UART5_MODE, "uart5", AIC_UART5_RTS_NAME,
     AIC_UART5_CTS_NAME, AIC_UART5_RS232_CTRL_NAME},
#endif
#ifdef AIC_USING_UART6
    {6, AIC_DEV_UART6_DATABITS, AIC_DEV_UART6_STOPBITS, AIC_DEV_UART6_PARITY, AIC_UART6_FLAG,
     AIC_DEV_UART6_BAUDRATE, AIC_CLK_UART6_FREQ, AIC_DEV_UART6_MODE, "uart6", AIC_UART6_RTS_NAME,
     AIC_UART6_CTS_NAME, AIC_UART6_RS232_CTRL_NAME},
#endif
#ifdef AIC_USING_UART7
    {7, AIC_DEV_UART7_DATABITS, AIC_DEV_UART7_STOPBITS, AIC_DEV_UART7_PARITY, AIC_UART7_FLAG,
     AIC_DEV_UART7_BAUDRATE, AIC_CLK_UART7_FREQ, AIC_DEV_UART7_MODE, "uart7", AIC_UART7_RTS_NAME,
     AIC_UART7_CTS_NAME, AIC_UART7_RS232_CTRL_NAME},
#endif
};

static void uart_halt_tx_irq_handler(void *args)
{
    usart_handle_t uart;
    unsigned int pin, value = PIN_LOW;
    struct rt_serial_device *serial  = args;

    uart  = (usart_handle_t)serial->parent.user_data;

    pin = uart_cts_dev[serial->config.uart_index].uart_cts_pin;
    value = rt_pin_read(pin);

    if (value == PIN_HIGH) {
        hal_usart_halt_tx_enable(uart, HALT_TX_ENABLE);
    } else {
        hal_usart_halt_tx_enable(uart, HALT_TX_DISABLE);
    }
}

void drv_usart_function_init(int i, int u)
{
    if ((rt_strncmp(uart_dev_paras[i].uart_rts_name, "no_pin", 6) != 0) &&
        (uart_dev_paras[i].function == USART_FUNC_RS485_SIMULATION ||
        uart_dev_paras[i].function == USART_MODE_RS232_UNAUTO_FLOW_CTRL ||
        uart_dev_paras[i].function == USART_MODE_RS232_SW_HW_FLOW_CTRL)) {

        uart_rts_dev[u].uart_rts_pin = rt_pin_get(uart_dev_paras[i].uart_rts_name);
        rt_pin_mode(uart_rts_dev[u].uart_rts_pin, PIN_MODE_OUTPUT);
        rt_pin_write(uart_rts_dev[u].uart_rts_pin, PIN_LOW);
        g_serial[u].config.flowctrl_rts_enable = 1;
    }

    if ((rt_strncmp(uart_dev_paras[i].uart_cts_name, "no_pin", 6) != 0) &&
        (uart_dev_paras[i].function == USART_MODE_RS232_UNAUTO_FLOW_CTRL ||
        uart_dev_paras[i].function == USART_MODE_RS232_SW_HW_FLOW_CTRL)) {

        uart_cts_dev[u].uart_cts_pin = rt_pin_get(uart_dev_paras[i].uart_cts_name);
        rt_pin_mode(uart_cts_dev[u].uart_cts_pin, PIN_MODE_INPUT);
        rt_pin_attach_irq(uart_cts_dev[u].uart_cts_pin, PIN_IRQ_MODE_RISING_FALLING,
                            uart_halt_tx_irq_handler, (void *)&(g_serial[u]));
        rt_pin_irq_enable(uart_cts_dev[u].uart_cts_pin, PIN_IRQ_ENABLE);
        g_serial[u].config.flowctrl_cts_enable = 1;
    }

    if ((rt_strncmp(uart_dev_paras[i].rs232_ctrl_name, "no_pin", sizeof("no_pin")) != 0) &&
        (uart_dev_paras[i].function == USART_FUNC_RS232_SIMULATION)) {
        uart_ctrl_dev[u].uart_ctrl_pin = rt_pin_get(uart_dev_paras[i].rs232_ctrl_name);
        rt_pin_mode(uart_ctrl_dev[u].uart_ctrl_pin, PIN_MODE_OUTPUT);
        rt_pin_write(uart_ctrl_dev[u].uart_ctrl_pin, PIN_LOW);
    }

    if (uart_dev_paras[i].function == USART_MODE_RS232_SW_FLOW_CTRL) {
        g_serial[u].config.flowctrl_cts_enable = 1;
        g_serial[u].config.flowctrl_rts_enable = 1;
    }
}

void drv_usart_set_freq(uint32_t baudrate, int u)
{
    int uart_freq;
    int config_num = sizeof(uart_freq_baud_list)/sizeof(uart_freq_baud_list[0]);

    for (int x = 0; x < config_num; x++) {
        if (baudrate == uart_freq_baud_list[x].baud) {
            uart_freq = uart_freq_baud_list[x].freq;
            hal_clk_set_freq(CLK_UART0 + u, uart_freq);
            return;
        }
    }
    /* Use the menuconfig freq when the baud dose not match in the list */
    hal_clk_set_freq(CLK_UART0 + u, g_serial[u].config.uart_freq);
}

#ifdef AIC_USING_PM
static int aic_usart_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    aic_usart_priv_t *uart_data = device->user_data;
    usart_handle_t uart = (usart_handle_t)uart_data;

#ifdef AIC_NO_CONSOLE_SUSPEND
    if(!rt_strncmp(device->parent.name, RT_CONSOLE_DEVICE_NAME, RT_NAME_MAX))
        return 0;
#endif

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_STANDBY:
        hal_usart_flush(uart, USART_FLUSH_WRITE);
        hal_clk_disable(CLK_UART0 + uart_data->idx);
        break;
    case PM_SLEEP_MODE_DEEP:
        hal_usart_flush(uart, USART_FLUSH_WRITE);
        hal_clk_disable(CLK_UART0 + uart_data->idx);
#if defined(AIC_PM_DRV_V15) || defined(AIC_PM_DRV_V14)
        hal_reset_assert(RESET_UART0 + uart_data->idx);
#endif
        break;
    default:
        break;
    }

    return 0;
}

static void aic_usart_resume(const struct rt_device *device, rt_uint8_t mode)
{
    aic_usart_priv_t *uart_data = device->user_data;
#if defined(AIC_PM_DRV_V15) || defined(AIC_PM_DRV_V14)
    struct rt_serial_device *serial = (struct rt_serial_device *)device;
    int i = 0;
#endif

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_NO_CONSOLE_SUSPEND
        if(!rt_strncmp(device->parent.name, RT_CONSOLE_DEVICE_NAME, RT_NAME_MAX))
            break;
#endif
        hal_clk_enable(CLK_UART0 + uart_data->idx);
        break;
    case PM_SLEEP_MODE_DEEP:
#if defined(AIC_PM_DRV_V15) || defined(AIC_PM_DRV_V14)
        hal_reset_deassert(RESET_UART0 + uart_data->idx);
        hal_clk_enable(CLK_UART0 + uart_data->idx);
        drv_uart_configure(serial, &serial->config);
        for (i = 0; i < sizeof(uart_dev_paras)/sizeof(struct drv_uart_dev_para); i++) {
            if (uart_dev_paras[i].index == uart_data->idx)
                break;
        }
#if defined (AIC_SERIAL_USING_DMA)
        if (uart_dev_paras[i].flag == AIC_UART_DMA_FLAG)
            hal_uart_set_fifo(uart_data);
#endif
        if (device->flag & RT_DEVICE_FLAG_INT_RX)
            hal_usart_set_interrupt((usart_handle_t)uart_data, USART_INTR_READ, 1);
        drv_usart_function_init(i, uart_data->idx);
#else
#ifdef AIC_NO_CONSOLE_SUSPEND
        if(!rt_strncmp(device->parent.name, RT_CONSOLE_DEVICE_NAME, RT_NAME_MAX))
            break;
#endif
        hal_clk_enable(CLK_UART0 + uart_data->idx);
#endif
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_usart_pm_ops =
{
    SET_LATE_DEVICE_PM_OPS(aic_usart_suspend, aic_usart_resume)
    NULL,
};
#endif

int drv_usart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    int u = 0;
    int i = 0;

    for (i=0; i<sizeof(uart_dev_paras)/sizeof(struct drv_uart_dev_para); i++) {
        u = uart_dev_paras[i].index;
        g_serial[u].ops                 = & drv_uart_ops;
        g_serial[u].config              = config;
        g_serial[u].config.bufsz        = 2048;
        g_serial[u].config.baud_rate    = uart_dev_paras[i].baud_rate;
        g_serial[u].config.data_bits    = uart_dev_paras[i].data_bits;
        g_serial[u].config.stop_bits    = uart_dev_paras[i].stop_bits - 1;
        g_serial[u].config.parity       = uart_dev_paras[i].parity;
        g_serial[u].config.function     = uart_dev_paras[i].function;
        g_serial[u].config.flag         = uart_dev_paras[i].flag;
        g_serial[u].config.uart_index   = uart_dev_paras[i].index;
        g_serial[u].config.uart_freq    = uart_dev_paras[i].clk_freq;

        drv_usart_set_freq(uart_dev_paras[i].baud_rate, u);
        hal_clk_enable(CLK_UART0 + u);
        hal_reset_assert(RESET_UART0 + u);
        aic_udelay(10000);
        hal_reset_deassert(RESET_UART0 + u);

#ifdef FINSH_POLL_MODE
        uart_handle[u] = hal_usart_initialize(u, NULL, NULL);
#else
        uart_handle[u] = hal_usart_initialize(u, drv_usart_cb_event, drv_usart_irqhandler);
#endif
        rt_hw_serial_register((struct rt_serial_device *)&g_serial[u],
                              uart_dev_paras[i].name,
#ifdef FINSH_POLL_MODE
                              RT_DEVICE_FLAG_RDWR,
#else
                              uart_dev_paras[i].flag,
#endif
                              uart_handle[u]);
#if defined (AIC_SERIAL_USING_DMA)
        if (uart_dev_paras[i].flag == AIC_UART_DMA_FLAG) {
            hal_uart_set_fifo((aic_usart_priv_t *)g_serial[u].parent.user_data);
            hal_uart_attach_callback((aic_usart_priv_t *)g_serial[u].parent.user_data,
                                        drv_uart_callback, NULL);
            hal_usart_set_interrupt((aic_usart_priv_t *)g_serial[u].parent.user_data,
                                        USART_INTR_READ, 1);

            g_info[u].uart_rx_fifo = (uint32_t *)aicos_malloc_align(0, AIC_UART_RX_FIFO_SIZE, UART_ALIGN_SIZE);
            g_info[u].uart_tx_fifo = (uint32_t *)aicos_malloc_align(0, AIC_UART_TX_FIFO_SIZE, UART_ALIGN_SIZE);
            if (!g_info[u].uart_rx_fifo || !g_info[u].uart_tx_fifo)
                return -RT_ENOMEM;

            g_info[u].tx_semaphore = rt_sem_create(uart_sem_name[u], 0, RT_IPC_FLAG_FIFO);
            if (!g_info[u].tx_semaphore)
                return -RT_ERROR;
        }
#endif
        drv_usart_function_init(i, u);
#ifdef AIC_USING_PM
        rt_pm_device_register((struct rt_device *)&g_serial[u].parent, &aic_usart_pm_ops);
#endif
    }

    return 0;
}
INIT_BOARD_EXPORT(drv_usart_init);
