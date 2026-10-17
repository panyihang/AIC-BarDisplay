/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zrq <ruiqi.zheng@artinchip.com>
 */

#include <aic_core.h>
#include <aic_hal_gpio.h>
#include <mmc.h>
#ifdef LPKG_USING_DFS
#include <dfs_fs.h>
#ifdef LPKG_USING_DFS_ELMFAT
#include <dfs_elm.h>
#endif
#endif

#define SD_CHECK_PIN ("PC.6")

static bool g_sdcard_hotplug_flag = 0;

irqreturn_t sdcard_detect_irq_handler(int irq, void *args)
{
    u8 pin;
    u8 g, p;
    u32 value = 0;

    pin = hal_gpio_name2pin(SD_CHECK_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_get_value(g, p, &value);
    hal_gpio_group_set_irq_stat(g, 0xFFFFFFFF);

    if (value) {
        printf("card removal detected!\n");
        g_sdcard_hotplug_flag = 1;
    } else {
        printf("card insertion detected!\n");
        g_sdcard_hotplug_flag = 0;
    }

    return IRQ_HANDLED;

}

void sdcard_detect_pin_init(void)
{
    u8 pin;
    u8 g, p;

    pin = hal_gpio_name2pin(SD_CHECK_PIN);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);

    hal_gpio_set_debounce(g, p, 0xFFF);
    hal_gpio_set_bias_pull(g, p, PIN_PULL_UP);
    hal_gpio_direction_input(g, p);

    hal_gpio_set_irq_mode(g, p, PIN_IRQ_MODE_EDGE_BOTH);

    aicos_request_irq(GPIO_IRQn + g, sdcard_detect_irq_handler, 0, "pin_group", NULL);
    aicos_irq_enable(GPIO_IRQn + g);
    hal_gpio_enable_irq(g, p);

}

void sdcard_hotplug_init(void)
{
    sdcard_detect_pin_init();
}

void sdcard_hotplug_act(void)
{
    static bool last_removal_stat = 0;

    if (g_sdcard_hotplug_flag && !last_removal_stat) {
        last_removal_stat = 1;
        mmc_deinit(1);
#if defined(LPKG_USING_DFS_ELMFAT) && defined(AIC_SDMC_DRV)
        if (dfs_unmount("/sdcard") < 0)
            printf("Failed to umount SD Card with FatFS\n");
#endif
    } else if (!g_sdcard_hotplug_flag && last_removal_stat) {
        last_removal_stat = 0;
        mmc_init(1);
#if defined(LPKG_USING_DFS_ELMFAT) && defined(AIC_SDMC_DRV)
        if (dfs_mount("sd1", "/sdcard", "elm", 0, DEVICE_TYPE_SDMC_DISK) < 0)
            printf("Failed to mount SD Card with FatFS\n");
#endif

    }
}
