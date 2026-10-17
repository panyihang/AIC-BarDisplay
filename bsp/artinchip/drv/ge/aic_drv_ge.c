/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ardon <haidong.pan@artinchip.com>
 */

#include <rtconfig.h>
#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <drivers/pm.h>
#endif
#include "aic_core.h"
#include "aic_common.h"
#include <aic_hal_ge.h>
#include "aic_hal_clk.h"
#include "aic_hal_reset.h"

#ifdef RT_USING_PM
struct rt_device ge_device = { 0 };

static int aic_ge_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    (void)device;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        if (hal_clk_is_enabled(CLK_GE))
#ifdef AIC_PM_DRV_V15
            hal_clk_disable_assertrst(CLK_GE);
#else
            hal_clk_disable(CLK_GE);
#endif
        break;
    default:
        break;
    }

    return 0;
}

static void aic_ge_resume(const struct rt_device *device, rt_uint8_t mode)
{
    (void)device;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        if (!hal_clk_is_enabled(CLK_GE))
#ifdef AIC_PM_DRV_V15
            hal_clk_enable_deassertrst(CLK_GE);
#else
            hal_clk_enable(CLK_GE);
#endif
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_ge_pm_ops =
{
    SET_DEVICE_PM_OPS(aic_ge_suspend, aic_ge_resume)
    NULL,
};
#endif

struct aic_ge_client *aic_ge_open(void)
{
    struct aic_ge_client *client = hal_ge_open();

    if (!client)
        return NULL;

    return client;
}

int aic_ge_close(struct aic_ge_client *client)
{
    if (!client)
        return -1;

    return hal_ge_close(client);
}

int aic_ge_write(struct aic_ge_client *client, const char *buff, size_t count)
{
    int ret = 0;

    if (!client)
        return -1;
#ifdef RT_USING_PM
    rt_pm_module_request(PM_GE_ID, PM_SLEEP_MODE_NONE);
#endif
    ret = hal_ge_write(client, buff, count);
#ifdef RT_USING_PM
    rt_pm_module_release(PM_GE_ID, PM_SLEEP_MODE_NONE);
#endif
    return ret;
}

int aic_ge_ioctl(struct aic_ge_client *client, int cmd, void *arg)
{
    int ret = 0;
    if (!client)
        return -1;
#ifdef RT_USING_PM
    rt_pm_module_request(PM_GE_ID, PM_SLEEP_MODE_NONE);
#endif
    ret = hal_ge_control(client, cmd, arg);
#ifdef RT_USING_PM
    rt_pm_module_release(PM_GE_ID, PM_SLEEP_MODE_NONE);
#endif
    return ret;
}

int aic_ge_probe(void)
{
    int ret;

    ret = hal_ge_init();
#ifdef RT_USING_PM
    rt_device_register(&ge_device, "ge", RT_DEVICE_FLAG_RDWR);
    rt_pm_device_register(&ge_device, &aic_ge_pm_ops);
#endif

    return ret;
}

#if defined(KERNEL_RTTHREAD)
INIT_DEVICE_EXPORT(aic_ge_probe);
#endif
