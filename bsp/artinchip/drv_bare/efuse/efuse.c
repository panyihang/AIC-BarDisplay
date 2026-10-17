/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <hal_efuse.h>
#include <efuse.h>

int efuse_init(void)
{
    int ret;

    ret = hal_efuse_init();
    if (ret) {
        pr_err("Failed to initialize efuse.\n");
        return -1;
    }

    return 0;
}

void efuse_write_enable(void)
{
    hal_efuse_write_enable();
}

void efuse_write_disable(void)
{
    hal_efuse_write_disable();
}

int efuse_read(u32 addr, void *data, u32 size)
{
    u32 wid, wval, rest, cnt;
    u8 *pd, *pw;
    int ret;

    if (hal_efuse_clk_enable()) {
        return -1;
    }

    if (hal_efuse_wait_ready()) {
        hal_efuse_clk_disable();
        pr_err("eFuse is not ready.\n");
        return -1;
    }

    pd = data;
    rest = size;
    while (rest > 0) {
        wid = addr >> 2;
        ret = hal_efuse_read(wid, &wval);
        if (ret)
            break;
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

int efuse_read_chip_id(void *data)
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
            efuse_read(0x10, data, 0x10);
            break;
        default:
            pr_err("not support read chip id, version 0x%x\n", version);
            return -1;
    }

    return 0;
}

#ifdef EFUSE_WRITE_SUPPORT
int efuse_program(u32 addr, const void *data, u32 size)
{
    u32 wid, wval, rest, cnt;
    const u8 *pd;
    u8 *pw;
    int ret;

    if (hal_efuse_clk_enable()) {
        return -1;
    }

    if (hal_efuse_wait_ready()) {
        hal_efuse_clk_disable();
        pr_err("eFuse is not ready.\n");
        return -1;
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

int sjtag_auth(u32 *key, u32 kwlen)
{
    int ret;

    if (hal_efuse_clk_enable()) {
        return -1;
    }

    ret = hal_sjtag_auth(key, kwlen);

    hal_efuse_clk_disable();

    return ret;
}

int szone_auth(u32 *key, u32 kwlen)
{
    int ret;

    if (hal_efuse_clk_enable()) {
        return -1;
    }

    ret = hal_szone_auth(key, kwlen);

    hal_efuse_clk_disable();

    return ret;
}
