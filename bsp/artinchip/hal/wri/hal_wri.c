/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "hal_wri.h"

#define  GTC_CNTVL       0x8
#define  GTC_CLK_FREQ    4000000

struct reboot_info {
    u32 inited;
    u32 reg_val;
    enum aic_reboot_reason reason;
    enum aic_reboot_reason sw_reason;
    enum aic_warm_reset_type hw_reason;
};

static struct reboot_info g_last_reboot = {0};
static const struct aic_wri_ops *ops = &wri_ops;

enum aic_warm_reset_type aic_wr_type_get(void)
{
    s32 i;
    u32 val = 0;

    if (g_last_reboot.inited)
        return g_last_reboot.hw_reason;

    val = readl(WRI_RST_FLAG);
    if (!val)
        return WRI_TYPE_ERR;

    g_last_reboot.reg_val = val;

    for (i = WRI_TYPE_MAX - 1; i >= 0; i--) {
        if (val & ops->wri_bit[i]) {
            g_last_reboot.hw_reason = (enum aic_warm_reset_type)i;
            return g_last_reboot.hw_reason;
        }
    }

    pr_warn("Invalid warm reset flag: %#x\n", val);
    return WRI_TYPE_ERR;
}

enum aic_reboot_reason aic_judge_reboot_reason(enum aic_warm_reset_type hw,
                                               u32 sw)
{
    int ret;
    enum aic_reboot_reason r = (enum aic_reboot_reason)sw;

    /* First, check the software-triggered reboot */
    if (wri_ops.is_wdt_reset(hw)) {
        wri_ops.sw_reboot_reason(sw, r);
        return r;
    }

    /* Second, check the hardware-triggered reboot */
    ret = wri_ops.hw_reboot_reason(hw, &r, sw);
    if (ret >= 0) {
        return (enum aic_reboot_reason)ret;
    }

    pr_warn("Unknown reset reason: %d - %d\n", hw, sw);
    return r;
}

void aic_set_reboot_reason(enum aic_reboot_reason r)
{
    u32 cur = 0;
    u8 reason_num = WRI_REBOOT_REASON_MASK >> WRI_REBOOT_REASON_SHIFT;

    cur = GET_REG_STATUS(REG_BOOT_INFO);
     /* If it's valid already, so ignore the current request */
    if (cur & WRI_REBOOT_REASON_MASK)
        return;

    BOOT_INFO_SET(r, WRI_REBOOT_REASON_MASK, WRI_REBOOT_REASON_SHIFT, cur);

    if (r <= reason_num)
        pr_debug("Set reset reason %d\n", r);
}

enum aic_reboot_reason aic_get_reboot_reason(void)
{
    u32 val = 0;
    enum aic_warm_reset_type hw = aic_wr_type_get();

    if (g_last_reboot.inited)
        return g_last_reboot.reason;

    BOOT_INFO_GET(WRI_REBOOT_REASON_MASK, WRI_REBOOT_REASON_SHIFT, val);

    pr_debug("Last reset info: hw %d, sw %d\n", hw, val);
    printf("Reset flag: 0x%x\n", g_last_reboot.reg_val);
    g_last_reboot.reason = aic_judge_reboot_reason(hw, val);
    g_last_reboot.inited = 1;

    return g_last_reboot.reason;
}

void aic_clr_reboot_reason(void)
{
    u32 val = 0;

    val = readl(WRI_RST_FLAG);
    writel(val, WRI_RST_FLAG); /* clear the flag */

    BOOT_INFO_GET(0xFF, 0, val);
    if (val & WRI_REBOOT_REASON_MASK) {
        /* Clear the previous state */
        clrbits(WRI_REBOOT_REASON_MASK, val);
        BOOT_INFO_WRITEB(val);
    }
    return;
}

/* Convert and print the time info from a GTC counter.
   It's useful when print the time before console is ready. */
void aic_show_gtc_time(char *tag, u32 val)
{
    u32 sec, msec;
    u32 cnt = val;

    if (!cnt)
        cnt = readl(GTC_BASE + GTC_CNTVL);

    sec = cnt / GTC_CLK_FREQ;
    msec = (cnt % GTC_CLK_FREQ) / 4 / 1000;
    printf("\n%s time: %d.%03d sec\n", tag ? tag : "GTC", sec, msec);
}

void aic_show_startup_time(void)
{
    u32 cnt = readl(GTC_BASE + GTC_CNTVL);

    aic_show_gtc_time("Startup", cnt);
}
