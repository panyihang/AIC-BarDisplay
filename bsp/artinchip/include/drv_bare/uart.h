/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BL_UART_H_
#define __BL_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rtconfig.h>
#include <aic_core.h>

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

#define AIC_UART_RX_MODE_POLL 0
#define AIC_UART_RX_MODE_INT  1
#define AIC_UART_RX_MODE_DMA  2

#ifndef AIC_DEV_UART0_RX_MODE
#define AIC_DEV_UART0_RX_MODE AIC_UART_RX_MODE_POLL
#endif
#ifndef AIC_DEV_UART1_RX_MODE
#define AIC_DEV_UART1_RX_MODE AIC_UART_RX_MODE_POLL
#endif
#ifndef AIC_DEV_UART2_RX_MODE
#define AIC_DEV_UART2_RX_MODE AIC_UART_RX_MODE_POLL
#endif
#ifndef AIC_DEV_UART3_RX_MODE
#define AIC_DEV_UART3_RX_MODE AIC_UART_RX_MODE_POLL
#endif
#ifndef AIC_DEV_UART4_RX_MODE
#define AIC_DEV_UART4_RX_MODE AIC_UART_RX_MODE_POLL
#endif
#ifndef AIC_DEV_UART5_RX_MODE
#define AIC_DEV_UART5_RX_MODE AIC_UART_RX_MODE_POLL
#endif
#ifndef AIC_DEV_UART6_RX_MODE
#define AIC_DEV_UART6_RX_MODE AIC_UART_RX_MODE_POLL
#endif
#ifndef AIC_DEV_UART7_RX_MODE
#define AIC_DEV_UART7_RX_MODE AIC_UART_RX_MODE_POLL
#endif

#ifndef AIC_DEV_UART0_RX_BUFSZ
#define AIC_DEV_UART0_RX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART1_RX_BUFSZ
#define AIC_DEV_UART1_RX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART2_RX_BUFSZ
#define AIC_DEV_UART2_RX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART3_RX_BUFSZ
#define AIC_DEV_UART3_RX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART4_RX_BUFSZ
#define AIC_DEV_UART4_RX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART5_RX_BUFSZ
#define AIC_DEV_UART5_RX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART6_RX_BUFSZ
#define AIC_DEV_UART6_RX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART7_RX_BUFSZ
#define AIC_DEV_UART7_RX_BUFSZ 64
#endif

#ifndef AIC_DEV_UART0_TX_BUFSZ
#define AIC_DEV_UART0_TX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART1_TX_BUFSZ
#define AIC_DEV_UART1_TX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART2_TX_BUFSZ
#define AIC_DEV_UART2_TX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART3_TX_BUFSZ
#define AIC_DEV_UART3_TX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART4_TX_BUFSZ
#define AIC_DEV_UART4_TX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART5_TX_BUFSZ
#define AIC_DEV_UART5_TX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART6_TX_BUFSZ
#define AIC_DEV_UART6_TX_BUFSZ 64
#endif
#ifndef AIC_DEV_UART7_TX_BUFSZ
#define AIC_DEV_UART7_TX_BUFSZ 64
#endif

int uart_init(int id);
int uart_config_update(int id, int baudrate);
int uart_deinit(int id);
int uart_getchar(int id);
int uart_putchar(int id, int c);
int uart_gets(int id, u8 *buf, int len);
int uart_puts(int id, u8 *buf, int len);
int uart_get_cur_baudrate(int id);
void uart_reset(int id);

#ifdef __cplusplus
}
#endif

#endif /* __BL_UART_H_ */
