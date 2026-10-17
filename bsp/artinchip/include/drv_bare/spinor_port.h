/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
 */

#ifndef __BL_SPINOR_H_
#define __BL_SPINOR_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <rtconfig.h>
#include <hal_qspi.h>

struct aic_qspi_bus
{
    char *name;
    u32 idx;
    u32 clk_id;
    u32 clk_in_hz;
    u32 bus_hz;
    u32 dma_port_id;
    u32 irq_num;
    u32 dl_width;
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    u32 cs_num;
#endif
    qspi_master_handle handle;
    int probe_flag;
    sfud_flash attached_flash;
    u32 rxd_dylmode;
    u32 txd_dylmode;
    u32 txc_dylmode;
};

#ifdef __cplusplus
}
#endif

#endif /* __BL_SPINOR_H_ */
