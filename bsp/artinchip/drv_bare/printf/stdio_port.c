/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <uart.h>
#ifdef AICUPG_LOG_BUFFER_SUPPORT
#include <log_buf.h>
#endif

#define PORT console_uart_id
static int console_uart_id = -1;
static int enable_uart_id = -1;

void stdio_set_uart(int id)
{
    console_uart_id = id;
    enable_uart_id = id;
}

void stdio_unset_uart(int id)
{
    if (id == enable_uart_id)
        enable_uart_id = -1;
}

int uart_putchar_port(int c)
{
    if (enable_uart_id == -1) {
#ifdef AIC_EARLY_DEBUG
        extern void early_debug_putc(char c);
        early_debug_putc(c);
#endif
        return c;
    } else {
        return uart_putchar(PORT, c);
    }
}

int putchar_port(int c)
{
#ifdef AICUPG_LOG_BUFFER_SUPPORT
    log_buf_write((char *)&c, 1);
#endif

    if (enable_uart_id == console_uart_id) {
        if (c == '\n')
            uart_putchar_port('\r');

        if (uart_putchar_port(c) < 0)
            return -1;
        return c;
    } else {
        return 0;
    }
}

int puts_port(const char *s)
{
    int cnt, c;

    cnt = 0;
    for (;;) {
        c = *s;
        if (c == 0)
            break;
        if (putchar_port(c) < 0)
            break;
        cnt++;
        s++;
    }
    putchar_port('\n');

    return cnt;
}

int getchar_port(void)
{
    if (enable_uart_id == console_uart_id) {
        return uart_getchar(PORT);
    } else {
        return 0;
    }
}

int getc(FILE *stream) __attribute__((alias("getchar_port")));
int getchar(void) __attribute__ ((alias("getchar_port")));
int puts(const char *s) __attribute__ ((alias("puts_port")));
int putc(int c, FILE *stream) __attribute__((alias("putchar_port")));
int putchar(int c) __attribute__ ((alias("putchar_port")));
