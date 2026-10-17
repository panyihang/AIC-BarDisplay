/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#ifndef __BL_XSPIPSRAM_H_
#define __BL_XSPIPSRAM_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <hal_xspi.h>

struct aic_xspi
{
    char *name;
    u32 idx;
    u32 clk_id;
    u32 clk_in_hz;
    u32 dma_port_id;
    u32 irq_num;
    hal_xspi_handle handle;
    bool inited;
};

#define PSRAM_INIT_OK 0
#define PSRAM_INIT_FAILED 1

u32 aic_xspi_psram_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_XSPIPSRAM_H_ */
