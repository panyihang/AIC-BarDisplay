/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_ADCIM_H_
#define _ARTINCHIP_HAL_ADCIM_H_

#include "aic_common.h"

int hal_adcim_init(void);
int hal_adcim_deinit(void);
int hal_adcim_calibration_set(unsigned int val);
s32 hal_adcim_probe(void);
u32 hal_adcim_auto_calibration(void);
int hal_adcim_adc2voltage(u16 *val, u32 cal_data, int scale, float def_voltage);
u32 hal_adcim_adc2caled(u32 adc_data);

#ifdef AIC_ADCIM_DM_DRV
void hal_adcdm_status_show(void);
void hal_adcdm_chan_show(void);
s32 hal_adcdm_chan_store(u32 ch);
void hal_adcdm_rtp_down_store(bool down);
ssize_t hal_adcdm_sram_write(int *buf, u32 offset, size_t count);
#endif

u32 hal_adcim_version(void);

#endif
