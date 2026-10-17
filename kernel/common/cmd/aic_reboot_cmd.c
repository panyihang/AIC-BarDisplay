/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include "aic_core.h"
#include "aic_reboot_reason.h"
#include <aic_hal.h>

#if defined(AIC_CONSOLE_BARE_DRV)
#include "console.h"
#endif

#ifdef RT_USING_FINSH
#include <string.h>
#include <stdlib.h>
#include <finsh.h>
#include <drivers/watchdog.h>
#include "aic_drv_wdt.h"

#define TIMEOUT         0

#endif /* RT_USING_FINSH */

#if defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)
static int clk_mod_reset(char *name)
{
    int32_t clk_id;

    clk_id = hal_clk_get_id(name);
    if (clk_id < 0) {
        pr_err("Get %s clk-id failed!\n", name);
        return -1;
    }
    pr_info("Reset module: %s (%d)\n", name, (u32)clk_id);

    if (hal_clk_disable_assertrst(clk_id) < 0) {
        pr_err("Disable %s clk failed!\n", name);
        return -1;
    }
    if (hal_clk_enable_deassertrst(clk_id) < 0) {
        pr_err("Enable %s clk failed!\n", name);
        return -1;
    }
    return 0;
}

static int do_clk_mod_reset(int argc, char *argv[])
{
    int next;

    if (argc < 2) {
        pr_err("Should input as follow:\n\t%s [clk-name]\n", argv[0]);
        return -1;
    }

    for (next = 1; next < argc; next++) {
        if (clk_mod_reset(argv[next]) < 0)
            pr_err("Reset clk %s failed!\n", argv[next]);
    }

    return 0;
}
#endif

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(do_clk_mod_reset, reset_mod, Reset device module.);
#elif defined(AIC_CONSOLE_BARE_DRV)
CONSOLE_CMD(reset_mod, do_clk_mod_reset, "Reset device module.");
#endif

#if defined(RT_USING_FINSH)
void rt_hw_cpu_reset()
{
    u32 timeout = TIMEOUT;
    rt_device_t wdt_dev = RT_NULL;
    rt_err_t ret;

    wdt_dev = rt_device_find("wdt");
    if (wdt_dev == RT_NULL) {
        LOG_E("Watchdog device not found!");
        return;
    }

    ret = rt_device_init(wdt_dev);
    if (ret != RT_EOK && ret != -RT_EBUSY) {
        LOG_E("Watchdog device init failed: %d", ret);
        return;
    }

    pr_info("Restarting system ...");
    aicos_msleep(100);
#ifdef AIC_WRI_DRV
    aic_set_reboot_reason(REBOOT_REASON_CMD_REBOOT);
#endif
    ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != RT_EOK) {
        LOG_E("Watchdog set timeout failed: %d", ret);
        return;
    }
    ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    if (ret != RT_EOK) {
        LOG_E("Watchdog start failed: %d", ret);
        return;
    }

    aicos_msleep(1000);
    LOG_W("Watchdog doesn't work!");
}

void cmd_reboot(int argc, char **argv)
{
    rt_hw_cpu_reset();
}

MSH_CMD_EXPORT_ALIAS(cmd_reboot, reboot, Reboot the system);
#endif /* RT_USING_FINSH */

#ifdef QEMU_RUN
void cmd_poweroff(int argc, char **argv)
{
    printf("QEMU will exit after 1sec ...\n");
    aicos_msleep(1000);
    *(int *)0x10002000 = 88;
}

MSH_CMD_EXPORT_ALIAS(cmd_poweroff, poweroff, Exit the QEMU system);
#endif
