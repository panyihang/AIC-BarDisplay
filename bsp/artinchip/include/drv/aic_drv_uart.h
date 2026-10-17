/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_DRV_UART_
#define _AIC_DRV_UART_

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_hal_uart.h"

#define UART_DEVICE_CTRL_CLEAN_RS485        0x30            /**< disable rs485 mode*/
#define UART_DEVICE_CTRL_SET_RS485          0x31            /**< enable rs485 normal mode*/
#define UART_DEVICE_CTRL_SET_RS485C         0x32            /**< enable rs485 compact io mode*/

int drv_usart_init(void);
int32_t drv_usart_target_init(int32_t idx, uint32_t *base, uint32_t *irq, void **handler);


#ifdef __cplusplus
}
#endif

#endif /* _AIC_DRV_UART_ */
