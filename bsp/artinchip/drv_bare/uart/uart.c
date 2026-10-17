/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <driver.h>
#include <uart.h>
#include <ringbuffer.h>

#define AIC_CLK_UART_MAX_FREQ 100000000 /* max 100M*/

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

#define DEVICE_FLAG_INT_RX 0x100 /**< INT mode on Rx */
#define DEVICE_FLAG_DMA_RX 0x200 /**< DMA mode on Rx */
#define DEVICE_FLAG_INT_TX 0x400 /**< INT mode on Tx */
#define DEVICE_FLAG_DMA_TX 0x800 /**< DMA mode on Tx */
#define SERIAL_MODE (DEVICE_FLAG_INT_RX)

#ifndef SERIAL_RB_BUFSZ
#define SERIAL_RB_BUFSZ 64
#endif

#define UART_EVENT_RX_IND     0x01 /* Rx indication */
#define UART_EVENT_TX_DONE    0x02 /* Tx complete   */
#define UART_EVENT_RX_DMADONE 0x03 /* Rx DMA transfer done */
#define UART_EVENT_TX_DMADONE 0x04 /* Tx DMA transfer done */
#define UART_EVENT_RX_TIMEOUT 0x05 /* Rx timeout    */

struct uart_rx_fifo {
    /* software fifo */
    struct ringbuffer rb;
    u8 buffer[] __attribute__((aligned(CACHE_LINE_SIZE)));
};

struct uart_tx_fifo {
    /* software fifo */
    struct ringbuffer rb;
    u8 buffer[] __attribute__((aligned(CACHE_LINE_SIZE)));
};

struct uart_dev_para {
    u32 id        : 4;
    u32 data_bits : 4;
    u32 stop_bits : 2;
    u32 parity    : 2;
    u32 rx_mode   : 2;
    u32 rx_bufsz;
    u32 tx_bufsz;
    u32 baud_rate;
    u32 clk;
    u32 clk_freq;
    u32 rst;
    u32 function;
    u32 flag;
    char *name;
};

struct aic_uart {
#if defined (AIC_SERIAL_USING_DMA)
    u8 dma_tx_buf[AIC_UART_TX_FIFO_SIZE] __attribute__((aligned(CACHE_LINE_SIZE)));
    u8 dma_rx_buf[AIC_UART_RX_FIFO_SIZE] __attribute__((aligned(CACHE_LINE_SIZE)));
    u32 rx_size;
    u32 tx_size;
    u32 tx_dma_working;
#endif
    usart_handle_t handle;
    struct uart_dev_para *para;
    struct uart_rx_fifo *rx_fifo;
    struct uart_tx_fifo *tx_fifo;
};

static struct uart_dev_para uart_dev_paras[] =
{
#ifdef AIC_USING_UART0
    {0, AIC_DEV_UART0_DATABITS, AIC_DEV_UART0_STOPBITS, AIC_DEV_UART0_PARITY,
        AIC_DEV_UART0_RX_MODE, AIC_DEV_UART0_RX_BUFSZ, AIC_DEV_UART0_TX_BUFSZ,
        AIC_DEV_UART0_BAUDRATE, CLK_UART0, AIC_CLK_UART0_FREQ, RESET_UART0,
        AIC_DEV_UART0_MODE, AIC_UART0_FLAG, "uart0"},
#endif
#ifdef AIC_USING_UART1
    {1, AIC_DEV_UART1_DATABITS, AIC_DEV_UART1_STOPBITS, AIC_DEV_UART1_PARITY,
        AIC_DEV_UART1_RX_MODE, AIC_DEV_UART1_RX_BUFSZ, AIC_DEV_UART1_TX_BUFSZ,
        AIC_DEV_UART1_BAUDRATE, CLK_UART1, AIC_CLK_UART1_FREQ, RESET_UART1,
        AIC_DEV_UART1_MODE, AIC_UART1_FLAG, "uart1"},
#endif
#ifdef AIC_USING_UART2
    {2, AIC_DEV_UART2_DATABITS, AIC_DEV_UART2_STOPBITS, AIC_DEV_UART2_PARITY,
        AIC_DEV_UART2_RX_MODE, AIC_DEV_UART2_RX_BUFSZ, AIC_DEV_UART2_TX_BUFSZ,
        AIC_DEV_UART2_BAUDRATE, CLK_UART2, AIC_CLK_UART2_FREQ, RESET_UART2,
        AIC_DEV_UART2_MODE, AIC_UART2_FLAG, "uart2"},
#endif
#ifdef AIC_USING_UART3
    {3, AIC_DEV_UART3_DATABITS, AIC_DEV_UART3_STOPBITS, AIC_DEV_UART3_PARITY,
        AIC_DEV_UART3_RX_MODE, AIC_DEV_UART3_RX_BUFSZ, AIC_DEV_UART3_TX_BUFSZ,
        AIC_DEV_UART3_BAUDRATE, CLK_UART3, AIC_CLK_UART3_FREQ, RESET_UART3,
        AIC_DEV_UART3_MODE, AIC_UART3_FLAG, "uart3"},
#endif
#ifdef AIC_USING_UART4
    {4, AIC_DEV_UART4_DATABITS, AIC_DEV_UART4_STOPBITS, AIC_DEV_UART4_PARITY,
        AIC_DEV_UART4_RX_MODE, AIC_DEV_UART4_RX_BUFSZ, AIC_DEV_UART4_TX_BUFSZ,
        AIC_DEV_UART4_BAUDRATE, CLK_UART4, AIC_CLK_UART4_FREQ, RESET_UART4,
        AIC_DEV_UART4_MODE, AIC_UART4_FLAG, "uart4"},
#endif
#ifdef AIC_USING_UART5
    {5, AIC_DEV_UART5_DATABITS, AIC_DEV_UART5_STOPBITS, AIC_DEV_UART5_PARITY,
        AIC_DEV_UART5_RX_MODE, AIC_DEV_UART5_RX_BUFSZ, AIC_DEV_UART5_TX_BUFSZ,
        AIC_DEV_UART5_BAUDRATE, CLK_UART5, AIC_CLK_UART5_FREQ, RESET_UART5,
        AIC_DEV_UART5_MODE, AIC_UART5_FLAG, "uart5"},
#endif
#ifdef AIC_USING_UART6
    {6, AIC_DEV_UART6_DATABITS, AIC_DEV_UART6_STOPBITS, AIC_DEV_UART6_PARITY,
        AIC_DEV_UART6_RX_MODE, AIC_DEV_UART6_RX_BUFSZ, AIC_DEV_UART6_TX_BUFSZ,
        AIC_DEV_UART6_BAUDRATE, CLK_UART6, AIC_CLK_UART6_FREQ, RESET_UART6,
        AIC_DEV_UART6_MODE, AIC_UART6_FLAG, "uart6"},
#endif
#ifdef AIC_USING_UART7
    {7, AIC_DEV_UART7_DATABITS, AIC_DEV_UART7_STOPBITS, AIC_DEV_UART7_PARITY,
        AIC_DEV_UART7_RX_MODE, AIC_DEV_UART7_RX_BUFSZ, AIC_DEV_UART7_TX_BUFSZ,
        AIC_DEV_UART7_BAUDRATE, CLK_UART7, AIC_CLK_UART7_FREQ, RESET_UART7,
        AIC_DEV_UART7_MODE, AIC_UART7_FLAG, "uart7"},
#endif
};

static void *uart_dev[AIC_UART_DEV_NUM];

static void uart_irqhandler(int irq, void * data)
{
    struct aic_uart *uart = NULL;
    int id = irq - UART0_IRQn;
    u8 status= 0;

    if (id >= AIC_UART_DEV_NUM)
        return;

    uart = uart_dev[id];
    if (!uart)
        return;

    status = hal_usart_get_irqstatus(id);
    if (uart->para->flag == AIC_UART_DMA_FLAG) {
#if defined (AIC_SERIAL_USING_DMA)
        struct uart_tx_fifo *tx_fifo = NULL;
        tx_fifo = uart->tx_fifo;
        switch (status) {
            case AIC_IIR_THR_EMPTY:
                hal_usart_set_interrupt(uart->handle, USART_INTR_WRITE, 0);
                if (uart->tx_dma_working)
                    return;

                uart->tx_size = ringbuf_len(&(tx_fifo->rb));
                if (uart->tx_size) {
                    if (uart->tx_size > AIC_UART_TX_FIFO_SIZE)
                        uart->tx_size = AIC_UART_TX_FIFO_SIZE;

                    ringbuf_out(&(uart->tx_fifo->rb), uart->dma_tx_buf, uart->tx_size);
                    aicos_dcache_clean_range(uart->dma_tx_buf, ALIGN_UP(AIC_UART_TX_FIFO_SIZE, CACHE_LINE_SIZE));
                    hal_uart_send_by_dma(uart->handle, uart->dma_tx_buf, uart->tx_size);
                    uart->tx_dma_working = 1;
                }
                break;
            case AIC_IIR_RECV_DATA:
            case AIC_IIR_CHAR_TIMEOUT:
                uart->rx_size = hal_usart_get_rx_fifo_num(uart->handle);
                if (uart->rx_size) {
                    hal_usart_set_interrupt(uart->handle, USART_INTR_READ, 0);
                    hal_uart_rx_dma_config(uart->handle, uart->dma_rx_buf, uart->rx_size);
                }
                break;
            default:
                break;
        }
#endif
    } else {
        struct uart_rx_fifo *rx_fifo = NULL;
        struct uart_tx_fifo *tx_fifo = NULL;
        int ch = -1;

        rx_fifo = uart->rx_fifo;
        tx_fifo = uart->tx_fifo;
        switch (status) {
            case AIC_IIR_THR_EMPTY:
                hal_usart_set_interrupt(uart->handle, USART_INTR_WRITE, 0);
                while (1)
                {
                    if (ringbuf_len(&(tx_fifo->rb)) > 0) {
                        ringbuf_out(&(tx_fifo->rb), &ch, 1);
                        if (hal_usart_putchar(uart->handle, ch) != 0)
                            break;
                    } else {
                        break;
                    }
                }
                break;
            case AIC_IIR_RECV_DATA:
            case AIC_IIR_CHAR_TIMEOUT:
                hal_usart_set_interrupt(uart->handle, USART_INTR_READ, 0);
                while (1)
                {
                    ch = hal_uart_getchar(uart->handle);
                    if (ch == -1) break;
                    ringbuf_in(&(rx_fifo->rb), &ch, 1);
                }
                hal_usart_set_interrupt(uart->handle, USART_INTR_READ, 1);

                break;
            case AIC_IIR_RECV_LINE:
                hal_usart_set_interrupt(uart->handle, USART_INTR_READ, 0);
                hal_usart_intr_recv_line(id, uart->handle);
                hal_usart_set_interrupt(uart->handle, USART_INTR_READ, 1);
                break;
            default:
                break;
        }
    }
}

#if defined (AIC_SERIAL_USING_DMA)
static void uart_dma_callback(aic_usart_priv_t *handle, void *arg)
{
    struct aic_uart *uart = NULL;
    unsigned long event = (unsigned long)arg;

    uart = uart_dev[handle->idx];
    if (!uart)
        return;

    switch(event) {
    case AIC_UART_TX_INT:
        uart->tx_dma_working = 0;
        if (ringbuf_len(&(uart->tx_fifo->rb)))
            hal_usart_set_interrupt(handle, USART_INTR_WRITE, 1);
        break;
    case AIC_UART_RX_INT:
        aicos_dcache_invalid_range(uart->dma_rx_buf, ALIGN_UP(AIC_UART_RX_FIFO_SIZE, CACHE_LINE_SIZE));
        ringbuf_in(&(uart->rx_fifo->rb), uart->dma_rx_buf, uart->rx_size);
        hal_usart_set_interrupt(handle, USART_INTR_READ, 1);
        break;
    default:
        hal_log_err("not support event\n");
        break;
    }
}
#endif

static int uart_clk_init(struct aic_uart *uart)
{
    s32 ret = 0;

    hal_clk_disable_assertrst(uart->para->clk);
    hal_clk_disable(uart->para->clk);

    hal_clk_set_freq(uart->para->clk, uart->para->clk_freq);
    ret = hal_clk_enable(uart->para->clk);
    if (ret < 0) {
        pr_err("UART%d clk enable failed!\n", uart->para->id);
        return ret;
    }

    ret = hal_reset_assert(uart->para->rst);
    if (ret < 0) {
        pr_err("UART%d reset assert failed!\n", uart->para->id);
        return ret;
    }

    aic_udelay(500);

    ret = hal_reset_deassert(uart->para->rst);
    if (ret < 0) {
        pr_err("UART%d reset deassert failed!\n", uart->para->id);
        return ret;
    }

    return ret;
}

static u32 get_parity(u32 cfg)
{
    switch (cfg) {
        case 2:
            return USART_PARITY_EVEN;
        case 1:
            return USART_PARITY_ODD;
        default:
            return USART_PARITY_NONE;
    }
}

static u32 get_stop_bits(u32 cfg)
{
    switch (cfg) {
        case 0:
            return USART_STOP_BITS_0_5;
        case 1:
            return USART_STOP_BITS_1;
        case 2:
            return USART_STOP_BITS_2;
        case 3:
            return USART_STOP_BITS_1_5;
        default:
            return USART_STOP_BITS_1;
    }
}

static u32 get_data_bits(u32 cfg)
{
    switch (cfg) {
        case 5:
            return USART_DATA_BITS_5;
        case 6:
            return USART_DATA_BITS_6;
        case 7:
            return USART_DATA_BITS_7;
        case 8:
            return USART_DATA_BITS_8;
        case 9:
            return USART_DATA_BITS_9;
    }
    return USART_DATA_BITS_8;
}

int uart_init(int id)
{
    struct aic_uart *uart = NULL;
    struct uart_dev_para *para = NULL;
    struct uart_rx_fifo *rx_fifo = NULL;
    struct uart_tx_fifo *tx_fifo = NULL;
    usart_handle_t handle = NULL;
    u32 data_bits, stop_bits, parity;
    int ret = -1, i;

    if (id < 0 || id > AIC_UART_DEV_NUM - 1) {
        pr_err("Invalid UART ID %d\n", id);
        return -1;
    }

    if (uart_dev[id]) {
        pr_info("UART%d was already inited\n", id);
        return 0;
    }

#if defined (AIC_SERIAL_USING_DMA)
    uart = aicos_malloc_align(0, sizeof(struct aic_uart), CACHE_LINE_SIZE);
#else
    uart = malloc(sizeof(struct aic_uart));
#endif

    if (!uart) {
        pr_err("Failed to malloc(%d)\n", (u32)sizeof(struct aic_uart));
        return -1;
    }
    memset(uart, 0, sizeof(struct aic_uart));

    for (i = 0; i < ARRAY_SIZE(uart_dev_paras); i++) {
        if (id == uart_dev_paras[i].id) {
            para = &uart_dev_paras[i];
            break;
        }
    }
    if (para == NULL) {
        ret = -ENODEV;
        goto err;
    }
    uart->para = para;

    uart_clk_init(uart);

    if (para->rx_mode != AIC_UART_RX_MODE_INT) {
        handle = hal_usart_initialize(id, NULL, NULL);
        hal_usart_set_interrupt(handle, USART_INTR_READ, 0);
        hal_usart_set_interrupt(handle, USART_INTR_WRITE, 0);
    } else {
        rx_fifo = (struct uart_rx_fifo *)malloc(sizeof(struct uart_rx_fifo) + para->rx_bufsz);
        if (rx_fifo == NULL) {
            ret = -ENOMEM;
            goto err;
        }
        memset(rx_fifo, 0, sizeof(struct uart_rx_fifo) + para->rx_bufsz);
        ringbuf_init(&(rx_fifo->rb), rx_fifo->buffer, para->rx_bufsz);
        uart->rx_fifo = rx_fifo;

        tx_fifo = (struct uart_tx_fifo *)malloc(sizeof(struct uart_tx_fifo) + para->tx_bufsz);
        if (tx_fifo == NULL) {
            ret = -ENOMEM;
            goto err;
        }
        memset(tx_fifo, 0, sizeof(struct uart_tx_fifo) + para->tx_bufsz);
        ringbuf_init(&(tx_fifo->rb), tx_fifo->buffer, para->tx_bufsz);
        uart->tx_fifo = tx_fifo;

        handle = hal_usart_initialize(id, NULL, uart_irqhandler);
        hal_usart_set_interrupt(handle, USART_INTR_READ, 1);
    }
    if (!handle) {
        ret = -ENODEV;
        goto err;
    }
    uart->handle = handle;

    data_bits = get_data_bits(para->data_bits);
    stop_bits = get_stop_bits(para->stop_bits);
    parity = get_parity(para->parity);
    ret = hal_usart_config(handle, para->baud_rate, USART_MODE_ASYNCHRONOUS,
                           parity, stop_bits, data_bits, para->function);
    if (ret != 0)
        goto err;

#if defined (AIC_SERIAL_USING_DMA)
    hal_uart_set_fifo(handle);
    hal_uart_attach_callback(handle, uart_dma_callback, NULL);
    memset(uart->dma_rx_buf, 0, AIC_UART_RX_FIFO_SIZE);
    memset(uart->dma_tx_buf, 0, AIC_UART_TX_FIFO_SIZE);
#endif

    uart_dev[id] = uart;
    return ret;

err:
    if (uart->rx_fifo != NULL) {
        free(uart->rx_fifo);
        uart->rx_fifo = NULL;
    }

    if (uart->tx_fifo != NULL) {
        free(uart->tx_fifo);
        uart->tx_fifo = NULL;
    }

    if (uart != NULL) {
#if defined (AIC_SERIAL_USING_DMA)
        aicos_free_align(0, uart);
#else
        free(uart);
#endif
        uart = NULL;
    }
    return ret;
}

int uart_config_update(int id, int baudrate)
{
    struct aic_uart *uart = NULL;
    struct uart_dev_para *para = NULL;
    u32 data_bits, stop_bits, parity;
    int freq, parent_clk_id, parent_freq;
    int cmu_div, uart_div, real_baudrate;
    u64 err, min_err = baudrate / 40;
    int ret = 0;

    uart = uart_dev[id];
    if (!uart)
        return -ENODEV;

    para = uart->para;
    parent_clk_id = hal_clk_get_parent(para->clk);
    parent_freq   = hal_clk_get_freq(parent_clk_id);
    freq = para->clk_freq;

#if defined(AIC_CMU_DRV_V12)
    for (cmu_div = 16; cmu_div > 0; cmu_div--) {
#else
    for (cmu_div = 32; cmu_div > 0; cmu_div--) {
#endif
        if ((parent_freq / cmu_div) > AIC_CLK_UART_MAX_FREQ)
            continue;
        for (uart_div = 1; uart_div < 65536; uart_div++) {
            real_baudrate = parent_freq / (cmu_div * 16 * uart_div);
            err = abs(baudrate - real_baudrate);
            if (err < min_err) {
                freq = parent_freq / cmu_div;
                min_err = err;
            }
        }
    }

    if (min_err >= (baudrate / 40)) {
        printf("Error is too large for this baud rate.\n");
        return -1;
    }

    hal_clk_set_freq(para->clk, freq);
    hal_clk_enable(para->clk);
    hal_reset_assert(para->rst);
    hal_reset_deassert(para->rst);

    data_bits = get_data_bits(para->data_bits);
    stop_bits = get_stop_bits(para->stop_bits);
    parity = get_parity(para->parity);
    hal_usart_set_interrupt(uart->handle, USART_INTR_READ, 0);
    ret = hal_usart_config(uart->handle, baudrate, USART_MODE_ASYNCHRONOUS,
                           parity, stop_bits, data_bits, para->function);
    if (ret) {
        pr_err("set baudrate failed.\n");
    }
    hal_usart_set_interrupt(uart->handle, USART_INTR_READ, 1);

    return ret;
}

int uart_deinit(int id)
{
    struct aic_uart *uart = NULL;
    int ret;

    uart = uart_dev[id];
    if (!uart)
        return -ENODEV;

    ret = hal_usart_uninitialize(uart->handle);

    if (uart->rx_fifo != NULL) {
        free(uart->rx_fifo);
        uart->rx_fifo = NULL;
    }

    if (uart->tx_fifo != NULL) {
        free(uart->tx_fifo);
        uart->tx_fifo = NULL;
    }

    if (uart != NULL) {
        free(uart);
        uart = NULL;
    }

    return ret;
}

void uart_reset(int id)
{
    struct aic_uart *uart = NULL;

    uart = uart_dev[id];

    if (uart->rx_fifo)
        ringbuf_reset(&(uart->rx_fifo->rb));
    if (uart->tx_fifo)
        ringbuf_reset(&(uart->tx_fifo->rb));
}

int uart_getchar(int id)
{
    struct aic_uart *uart = NULL;
    struct uart_rx_fifo *rx_fifo = NULL;
    u64 start = 0, delta = 0;
    int timeout = 0;
    int ch = 0;

    uart = uart_dev[id];
    if (!uart)
        return -ENODEV;

    rx_fifo = uart->rx_fifo;

    if (rx_fifo == NULL) {
        return hal_uart_getchar(uart->handle);
    } else {
        start = aic_get_time_ms();
        while ((ringbuf_len(&(rx_fifo->rb)) < 1) && (delta < timeout))
        {
            delta = aic_get_time_ms() - start;
        }
        if (!ringbuf_out(&(rx_fifo->rb), &ch, 1))
            return -1;
    }

    return ch;
}

int uart_gets(int id, u8 *buf, int len)
{
    struct aic_uart *uart = NULL;
    struct uart_rx_fifo *rx_fifo = NULL;
    u64 start = 0, delta = 0;
    int i, timeout = 0;

    uart = uart_dev[id];
    if (!uart)
        return -ENODEV;

    rx_fifo = uart->rx_fifo;

    if (rx_fifo == NULL) {
        for (i = 0; i < len; i++) {
            start = aic_get_time_ms();
            do {
                buf[i] = hal_uart_getchar(uart->handle);
                delta = aic_get_time_ms() - start;
            } while (delta < timeout && buf[i] == -1);
            if (buf[i] == -1)
                break;
        }
        return i;
    } else {
        start = aic_get_time_ms();
        while ((ringbuf_len(&(rx_fifo->rb)) < len) && (delta < timeout))
        {
            delta = aic_get_time_ms() - start;
        }
        return ringbuf_out(&(rx_fifo->rb), buf, len);
    }
}

int uart_putchar(int id, int c)
{
    struct aic_uart *uart = NULL;
    struct uart_tx_fifo *tx_fifo = NULL;
    u64 start = 0, delta = 0;
    int timeout = 0;

    uart = uart_dev[id];
    if (!uart)
        return -ENODEV;

    tx_fifo = uart->tx_fifo;

    if (tx_fifo == NULL) {
        return hal_usart_putchar(uart->handle, (uint8_t)c);
    } else {
        start = aic_get_time_ms();
        while ((ringbuf_avail(&(tx_fifo->rb))) < 1 && (delta < timeout))
        {
            delta = aic_get_time_ms() - start;
        }

        if (ringbuf_in(&(tx_fifo->rb), &c, 1) <= 0)
            return -1;
        hal_usart_set_interrupt(uart->handle, USART_INTR_WRITE, 1);
    }

    return 0;
}

int uart_puts(int id, u8 *buf, int len)
{
    struct aic_uart *uart = NULL;
    struct uart_tx_fifo *tx_fifo = NULL;
    u64 start = 0, delta = 0;
    int i, timeout = 0;

    uart = uart_dev[id];
    if (!uart)
        return -ENODEV;

    tx_fifo = uart->tx_fifo;

    if (tx_fifo == NULL) {
        for (i = 0; i < len; i++) {
            if (hal_usart_putchar(uart->handle, (uint8_t)buf[i]))
                break;
        }
        return i;
    } else {
        start = aic_get_time_ms();
        while ((ringbuf_avail(&(tx_fifo->rb))) < len && (delta < timeout))
        {
            delta = aic_get_time_ms() - start;
        }
        i = ringbuf_in(&(tx_fifo->rb), buf, len);
        if (i <= 0)
            return 0;

        hal_usart_set_interrupt(uart->handle, USART_INTR_WRITE, 1);
        return i;
    }
}

int uart_get_cur_baudrate(int id)
{
    usart_handle_t handle = NULL;

    handle = hal_usart_initialize(id, NULL, NULL);
    return hal_usart_get_cur_baudrate(handle);
}
