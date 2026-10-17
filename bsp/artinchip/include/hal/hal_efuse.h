/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_EFUSE_H__
#define _ARTINCHIP_HAL_EFUSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

int hal_efuse_init(void);
int hal_efuse_deinit(void);
int hal_efuse_clk_enable(void);
int hal_efuse_clk_disable(void);
void hal_efuse_write_enable(void);
void hal_efuse_write_disable(void);
int hal_efuse_get_version(void);
int hal_efuse_wait_ready(void);
int hal_efuse_read(u32 wid, u32 *wval);
int hal_efuse_write(u32 wid, u32 wval);
int hal_sjtag_auth(u32 *key, u32 kwlen);
int hal_szone_auth(u32 *key, u32 kwlen);

#ifdef __cplusplus
}
#endif

#endif
