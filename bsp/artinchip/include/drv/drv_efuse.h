/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _ARTINCHIP_DRV_EFUSE_H__
#define _ARTINCHIP_DRV_EFUSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

void drv_efuse_write_enable(void);
void drv_efuse_write_disable(void);
int drv_efuse_read(u32 addr, void *data, u32 size);
int drv_efuse_read_chip_id(void *data);
int drv_efuse_read_reserved_1(void *data);
int drv_efuse_read_reserved_2(void *data);
int drv_efuse_program(u32 addr, const void *data, u32 size);
int drv_sjtag_auth(u32 *key, u32 kwlen);
int drv_szone_auth(u32 *key, u32 kwlen);

#ifdef __cplusplus
}
#endif

#endif
