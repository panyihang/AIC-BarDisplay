/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_errno.h>
#include <aic_hal.h>
#include "hal_wdt.h"
#include <wdt.h>

#define WDT_MAX_TIMEOUT_MS   (3600000) /* 1 hour */

int wdt_init(void)
{
    int ret;

    ret = hal_clk_enable(CLK_WDT);
    if (ret < -1) {
        pr_err("Watchdog clk enable failed.");
        return -EINVAL;
    }

    ret = hal_reset_assert(RESET_WDT);
    if (ret < -1) {
        pr_err("Watchdog reset assert failed.");
        return -EINVAL;
    }

    ret = hal_reset_deassert(RESET_WDT);
    if (ret < -1) {
        pr_err("Watchdog reset deassert failed.");
        return -EINVAL;
    }

    return 0;
}

int wdt_deinit(void)
{
    hal_clk_disable_assertrst(CLK_WDT);
    return 0;
}

int wdt_start(u32 tmo_ms)
{
    struct aic_wdt dog;
    u32 tmo_s, cfg_id;

    if (tmo_ms > WDT_MAX_TIMEOUT_MS) {
        pr_err("Max timeout is 1hour\n");
		return -EINVAL;
    }

    tmo_s = tmo_ms / 1000;
    dog.timeout = tmo_s;
    dog.rst_thd = tmo_s;

    cfg_id = 0;
    hal_wdt_rst_thd_set(cfg_id, &dog);

	/* Reset counter and reload cfg */
    hal_wdt_op_clr(0);

	/* Enable watchdog */
    hal_wdt_enable(1, 0);

    return 0;
}

void wdt_stop(void)
{
    hal_wdt_enable(0, 0);
}

int wdt_expire_now(void)
{
    return wdt_start(0);
}
