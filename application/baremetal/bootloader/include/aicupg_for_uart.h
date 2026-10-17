/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BL_AICUPG_FOR_UART_H_
#define __BL_AICUPG_FOR_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

void aic_uart_upg_init(int id);
void aic_uart_upg_deinit(int id);
void aic_uart_upg_loop(int id);

#ifdef __cplusplus
}
#endif

#endif /* __BL_AICUPG_FOR_UART_H_ */
