/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#include <rtconfig.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <hal_dma.h>
#include <hal_axicfg.h>
#include "axicfg_hw_v1.0.h"
#include "aic_hal_reset.h"


int hal_axicfg_module_wr_init(u8 mode, hal_axicfg_port_t device_p, u8 priority)
{
    // read: mode = 0; write: mode = 1;
    axicfg_hw_cfg0_set_reulator_en(HAL_AXICFG_BASE, mode, (u8)device_p, 0);
    axicfg_hw_cfg0_set_slv_ready(HAL_AXICFG_BASE, mode, (u8)device_p, 0);
    axicfg_hw_cfg0_set_burst_limit(HAL_AXICFG_BASE, mode, (u8)device_p, 0);
    axicfg_hw_cfg0_set_qos_sel(HAL_AXICFG_BASE, mode, (u8)device_p, HAL_AXICFG_INT_QOS);
    axicfg_hw_cfg0_set_qos_val(HAL_AXICFG_BASE, mode, (u8)device_p, priority);
    axicfg_hw_cfg1_set_basic_rate(HAL_AXICFG_BASE, mode, (u8)device_p, 0);
    axicfg_hw_cfg1_set_burst_rate(HAL_AXICFG_BASE, mode, (u8)device_p, 0);

    return 0;
}

int hal_axicfg_module_init(hal_axicfg_port_t device_p, u8 priority)
{
    u8 mode = 0;

    for (mode = 0; mode < 2; mode++) {
        // read: mode = 0; write: mode = 1;
        hal_axicfg_module_wr_init(mode, device_p, priority);
    }

    return 0;
}
