/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_WDT_H__
#define _ARTINCHIP_HAL_WDT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_common.h"

#define AIC_WDT_NAME            "aic-wdt"
#define AIC_WDT_CHAN_NUM        1

#define WDT_MAX_TIMEOUT         (60 * 60)
#define WDT_MIN_TIMEOUT         1
#define WDT_DEFAULT_TIMEOUT     10

#define WREG_PROTECT_EN         1
#define WREG_PROTECT_DIS        0

#ifdef AIC_WDT_DRV_V11
#define RST_CPU                 1
#define RST_SYS                 0

#endif
struct aic_wdt {
    u32 timeout; /* second */
    u32 clr_thd;
    u32 irq_thd;
    u32 rst_thd;
};

typedef s32 (*wdt_callback_t)(void);

void hal_wdt_register_callback(wdt_callback_t callback);
irqreturn_t hal_wdt_irq(int irq, void *arg);

void hal_wdt_op_clr(u32 thd);
s32 hal_wdt_is_running(void);
void hal_wdt_clr_thd_set(u32 ch, struct aic_wdt *wdt);
void hal_wdt_irq_thd_set(u32 ch, struct aic_wdt *wdt);
void hal_wdt_rst_thd_set(u32 ch, struct aic_wdt *wdt);
void hal_wdt_switch_chan(int chan);

u32 hal_wdt_remain(struct aic_wdt *wdt);
void hal_wdt_enable(u32 enable, u32 dbg_continue);
void hal_wdt_irq_enable(u32 enable);
int hal_wdt_irq_sta(void);
void hal_wdt_thd_get(u32 ch, struct aic_wdt *wdt);
int hal_wdt_clr_int(void);

void hal_wdt_status_show(u32 ch);
void hal_wdt_reg_protect(u8 enable);

#ifdef AIC_WDT_DRV_V11
void hal_wdt_rst_type_set(u32 rst);
int hal_wdt_rst_type_get(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
