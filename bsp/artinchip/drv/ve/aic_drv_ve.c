/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: <qi.xu@artinchip.com>
 */

#include <rtconfig.h>
#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_drv.h>
static struct rt_device ve_dev;
#endif
#include "aic_hal_ve.h"

struct aic_ve_client *drv_ve_open(void)
{
    return hal_ve_open();
}

int drv_ve_close(struct aic_ve_client *client)
{
    return hal_ve_close(client);
}

int drv_ve_control(struct aic_ve_client *client, int cmd, void *arg)
{
    return hal_ve_control(client, cmd, arg);
}

#ifdef AIC_USING_PM

#define VE_SRAM_MAP 0x164
static rt_bool_t g_ve_clk_suspended = RT_FALSE;

static int aic_ve_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    int ret = 0;
    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        if (hal_clk_is_enabled(CLK_VE)) {
            ret = hal_ve_close_with_wait();
            if (ret) {
                return ret;
            }
#ifdef AIC_PM_DRV_V15
            hal_clk_disable_assertrst(CLK_VE);
#else
            hal_clk_disable(CLK_VE);
#endif
            g_ve_clk_suspended = RT_TRUE;
        }
        break;
    default:
        break;
    }
    return ret;
}

static void aic_ve_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        if (g_ve_clk_suspended && !hal_clk_is_enabled(CLK_VE)) {
#ifdef AIC_PM_DRV_V15
            hal_clk_enable_deassertrst(CLK_VE);
#else
            hal_clk_enable(CLK_VE);
#endif
#if defined(AIC_VE_DRV_V40) || defined(AIC_VE_DRV_V31)
            // switch system sram to VE
            writel(1 | (0xa1c<<20), SYSCFG_BASE + VE_SRAM_MAP);
#endif
        }
        if (g_ve_clk_suspended) {
            g_ve_clk_suspended = RT_FALSE;
        }
        hal_ve_open();
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_ve_pm_ops = {
    SET_DEVICE_PM_OPS(aic_ve_suspend, aic_ve_resume)
    NULL,
};
#endif

#if defined(KERNEL_RTTHREAD)
int drv_ve_init(void)
{
    int ret;

    ret = hal_ve_probe();
    if (ret) {
        return ret;
    }

    rt_err_t rt_ret = rt_device_register(&ve_dev, "ve", RT_DEVICE_FLAG_DEACTIVATE);
    if (rt_ret != RT_EOK) {
        return -1;
    }

#ifdef AIC_USING_PM
    rt_pm_device_register(&ve_dev, &aic_ve_pm_ops);
#endif
    return ret;
}
INIT_DEVICE_EXPORT(drv_ve_init);
#endif
