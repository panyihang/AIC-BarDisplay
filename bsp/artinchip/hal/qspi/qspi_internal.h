/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_HAL_QSPI_INTERNAL_H_
#define _AIC_HAL_QSPI_INTERNAL_H_
#include <aic_common.h>
#include <aic_soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QSPI_WORK_MODE_SYNC_RX_CPU          0
#define QSPI_WORK_MODE_SYNC_TX_CPU          1
#define QSPI_WORK_MODE_ASYNC_RX_CPU         2
#define QSPI_WORK_MODE_ASYNC_TX_CPU         3
#define QSPI_WORK_MODE_ASYNC_RX_DMA         4
#define QSPI_WORK_MODE_ASYNC_TX_DMA         5
#define QSPI_WORK_MODE_SYNC_DUPLEX_CPU      6
#define QSPI_WORK_MODE_SYNC_DUPLEX_DMA      7
#define QSPI_WORK_MODE_ASYNC_DUPLEX_CPU     8
#define QSPI_WORK_MODE_ASYNC_DUPLEX_DMA     9
#define QSPI_WORK_MODE_FLUSH_DISPLAY        10

#define HAL_QSPI_STATUS_INTERNAL_MSK        (0xFFFFUL << 16)
#define HAL_QSPI_STATUS_ASYNC_TDONE         (0x1UL << 16)
#define HAL_QSPI_STATUS_ASYNC_DMA_DONE      (0x1UL << 17)
#define HAL_QSPI_STATUS_ASYNC_ALL_DONE      (HAL_QSPI_STATUS_ASYNC_TDONE | HAL_QSPI_STATUS_ASYNC_DMA_DONE)
#define QSPI_IS_ASYNC_ALL_DONE(sts, msk)    ((sts & msk) == msk)
#define PTR2U32(p) ((u32)(unsigned long)(p))

#define QSPI_TRANSFER_DATA_LEN_1M   0x100000
#define QSPI_CPU_DMA_MIN_SPEED_MS   (0x800000 >> 10)

void qspi_reg_dump(u32 base);
void show_freq(char *msg, u32 id, u32 hz);
u32 qspi_calc_timeout(u32 bus_hz, u32 len);
u32 qspi_master_get_best_div_param(u32 sclk, u32 bus_hz, u32 *div);

int qspi_fifo_read_data(u32 base, u8 *data, u32 len, u32 tmo_us);

int qspi_wait_transfer_done(u32 base, u32 tmo);

#ifdef __cplusplus
}
#endif

#endif
