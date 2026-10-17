/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#define LOG_TAG "SID"
#include <string.h>
#include <hal_efuse.h>
#include <drv_efuse.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <rtdevice.h>
#include <drivers/pm.h>

static struct rt_device efuse_dev = { 0 };
#ifdef AIC_USING_PM
static rt_bool_t g_efuse_clk_pm_flag = RT_FALSE;

static int drv_efuse_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode) {
        case PM_SLEEP_MODE_IDLE:
            break;
        case PM_SLEEP_MODE_LIGHT:
        case PM_SLEEP_MODE_DEEP:
        case PM_SLEEP_MODE_STANDBY:
            if (hal_clk_is_enabled(CLK_SID)) {
#ifdef AIC_PM_DRV_V15
                hal_clk_disable_assertrst(CLK_SID);
#else
                hal_clk_disable(CLK_SID);
#endif
                g_efuse_clk_pm_flag = RT_TRUE;
            }
            break;
        default:
            break;
    }
    return 0;
}

static void drv_efuse_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode) {
        case PM_SLEEP_MODE_IDLE:
            break;
        case PM_SLEEP_MODE_LIGHT:
        case PM_SLEEP_MODE_DEEP:
        case PM_SLEEP_MODE_STANDBY:
            if (g_efuse_clk_pm_flag && !hal_clk_is_enabled(CLK_SID)) {
#ifdef AIC_PM_DRV_V15
                hal_clk_enable_deassertrst(CLK_SID);
#else
                hal_clk_enable(CLK_SID);
#endif
            }
            if (g_efuse_clk_pm_flag)
                g_efuse_clk_pm_flag = RT_FALSE;
            break;
        default:
            break;
    }
}

static struct rt_device_pm_ops drv_efuse_pm_ops = {
    SET_DEVICE_PM_OPS(drv_efuse_suspend, drv_efuse_resume)
    NULL,
};
#endif

static int drv_efuse_init(void)
{
    int ret;

    ret = hal_efuse_init();
    if (ret) {
        LOG_E("Failed to initialize efuse.\n");
        return RT_FALSE;
    }

#ifdef KERNEL_RTTHREAD
    rt_device_register(&efuse_dev, "efuse", RT_DEVICE_FLAG_DEACTIVATE);
#ifdef AIC_USING_PM
    rt_pm_device_register(&efuse_dev, &drv_efuse_pm_ops);
#endif
#endif

    return RT_TRUE;
}

void drv_efuse_write_enable(void)
{
    hal_efuse_write_enable();
}

void drv_efuse_write_disable(void)
{
    hal_efuse_write_disable();
}

int drv_efuse_read(u32 addr, void *data, u32 size)
{
    u32 wid, wval, rest, cnt;
    u8 *pd, *pw;
    int ret = 0;

    if (data == NULL) {
        LOG_E("invalid data addr.\n");
        return RT_FALSE;
    }

    if (hal_efuse_clk_enable()) {
        return RT_FALSE;
    }

    if (hal_efuse_wait_ready()) {
        hal_efuse_clk_disable();
        LOG_E("eFuse is not ready.\n");
        return RT_FALSE;
    }

    pd = data;
    rest = size;
    while (rest > 0) {
        wid = addr >> 2;
        ret = hal_efuse_read(wid, &wval);
        if (ret) {
            hal_efuse_clk_disable();
            return ret;
        }
        pw = (u8 *)&wval;
        cnt = rest;
        if (addr % 4) {
            if (rest > (4 - (addr % 4)))
                cnt = (4 - (addr % 4));
            memcpy(pd, pw + (addr % 4), cnt);
        } else {
            if (rest > 4)
                cnt = 4;
            memcpy(pd, pw, cnt);
        }
        pd += cnt;
        addr += cnt;
        rest -= cnt;
    }

    hal_efuse_clk_disable();

    return (int)(size - rest);
}

int drv_efuse_read_chip_id(void *data)
{
    int version = 0;

    version = hal_efuse_get_version();
    switch (version) {
        case 0x100:
        case 0x101:
        case 0x102:
        case 0x103:
        case 0x105:
        case 0x107:
            drv_efuse_read(0x10, data, 0x10);
            break;
        default:
            pr_err("not support read chip id, version 0x%x\n", version);
            return -1;
    }

    return 0;
}

int drv_efuse_read_reserved_1(void *data)
{
    int version = 0;

    version = hal_efuse_get_version();
    switch (version) {
        case 0x100:
        case 0x101:
        case 0x102:
        case 0x103:
        case 0x105:
            drv_efuse_read(0x90, data, 0x10);
            break;
        default:
            pr_err("not support read reserved 1");
            return -1;
    }
    return 0;
}

int drv_efuse_read_reserved_2(void *data)
{
    int version = 0;

    version = hal_efuse_get_version();
    switch (version) {
        case 0x100:
        case 0x101:
        case 0x102:
        case 0x103:
        case 0x105:
            drv_efuse_read(0xC0, data, 0x40);
            break;
        default:
            pr_err("not support read reserved 2");
            return -1;
    }

    return 0;
}

#ifdef EFUSE_WRITE_SUPPORT
int drv_efuse_program(u32 addr, const void *data, u32 size)
{
    u32 wid, wval, rest, cnt;
    const u8 *pd;
    u8 *pw;
    int ret;

    if (hal_efuse_clk_enable()) {
        return RT_FALSE;
    }

    if (hal_efuse_wait_ready()) {
        hal_efuse_clk_disable();
        LOG_E("eFuse is not ready.\n");
        return RT_FALSE;
    }

    pd = data;
    rest = size;
    while (rest > 0) {
        cnt = rest;
        wval = 0;
        pw = (u8 *)&wval;
        if (addr % 4) {
            if (rest > (4 - (addr % 4)))
                cnt = (4 - (addr % 4));
            memcpy(pw + (addr % 4), pd, cnt);
        } else {
            if (rest > 4)
                cnt = 4;
            memcpy(pw, pd, cnt);
        }

        wid = addr >> 2;
        ret = hal_efuse_write(wid, wval);
        if (ret)
            break;
        pd += cnt;
        addr += cnt;
        rest -= cnt;
    }

    hal_efuse_clk_disable();

    return (int)(size - rest);
}
#endif

int drv_sjtag_auth(u32 *key, u32 kwlen)
{
    int ret;

    if (hal_efuse_clk_enable()) {
        return RT_FALSE;
    }

    ret = hal_sjtag_auth(key, kwlen);

    hal_efuse_clk_disable();

    return ret;
}

int drv_szone_auth(u32 *key, u32 kwlen)
{
    int ret;

    if (hal_efuse_clk_enable()) {
        return RT_FALSE;
    }

    ret = hal_szone_auth(key, kwlen);

    hal_efuse_clk_disable();

    return ret;
}

INIT_DEVICE_EXPORT(drv_efuse_init);
