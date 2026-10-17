/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_TSEN_H_
#define _ARTINCHIP_HAL_TSEN_H_

#include <stdbool.h>

#include "aic_common.h"
#include "aic_osal.h"

enum aic_tsen_mode {
    AIC_TSEN_MODE_SINGLE = 0,
    AIC_TSEN_MODE_PERIOD = 1
};

enum aic_tsen_soft_mode {
    AIC_TSEN_SOFT_MODE_SINGLE = 0,
    AIC_TSEN_SOFT_MODE_PERIOD = 1,
    AIC_TSEN_SOFT_MODE_POLLING = 2
};

enum aic_tsen_sensor {
    AIC_TSEN_SENSOR_CPU = 0,
    AIC_TSEN_SENSOR_GPAI = 1
};

#define AIC_TSEN_USE_DIFFERENT_MODE  1
#define AIC_TSEN_USE_INVERTED        1

struct aic_tsen_ch {
    int id;
    bool available;
    char name[16];
    enum aic_tsen_mode mode;
    enum aic_tsen_soft_mode soft_mode;
    bool diff_mode;
    bool inverted;
    u16  latest_data; // 10 * actual temperature value
    u16  smp_period; // in seconds
    u32 pclk_rate;

    bool hta_enable; // high temperature alarm
    u16  hta_thd;    // 10 * temperature value
    u16  hta_rm_thd; // 10 * temperature value
    bool lta_enable; // low temperature alarm
    u16  lta_thd;    // 10 * temperature value
    u16  lta_rm_thd; // 10 * temperature value
    bool otp_enable; // over temperature protection
    u16  otp_thd;    // 10 * temperature value

    int slope;       // 10000 * actual slope
    int offset;      // 10000 * actual offset

    aicos_sem_t complete;
};

void hal_tsen_enable(int enable);
void hal_tsen_ch_enable(u32 ch, int enable);
int hal_tsen_ch_init(struct aic_tsen_ch *chan, u32 pclk);

int hal_tsen_get_temp(struct aic_tsen_ch *chan, s32 *val);
s32 hal_tsen_data2temp(struct aic_tsen_ch *chan);
u16 hal_tsen_temp2data(struct aic_tsen_ch *chan, s32 temp);
void hal_tsen_status_show(struct aic_tsen_ch *chan);

irqreturn_t hal_tsen_irq_handle(int irq, void *arg);
s32 hal_tsen_clk_init(void);
void hal_tsen_pclk_get(struct aic_tsen_ch *chan);
void hal_tsen_curve_fitting(struct aic_tsen_ch *chan);
void hal_tsen_set_ch_num(u32 num);
struct aic_tsen_ch *hal_tsen_get_valid_ch(u32 ch);

#endif // end of _ARTINCHIP_HAL_TSEN_H_
