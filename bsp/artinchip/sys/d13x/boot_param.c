/*
 * Copyright (C) 2023-2024 ArtInChip Technology Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Dehuang Wu <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <aic_common.h>
#include <boot_param.h>

/*
 * Save boot parameters and context when save_boot_params is called.
 */
union boot_params boot_params_stash __attribute__((section(".data")));
struct boot_args boot_arg __attribute__((section(".data")));

enum boot_reason aic_get_boot_reason(void)
{
    enum boot_reason reason;

    /* SPL use a0 */
    reason = get_boot_reason(boot_params_stash.r.a[0]);
    return reason;
}

static const char *const boot_device_name[] = {
    "BD_NONE",   "BD_SDMC0",   "BD_SDMC1",   "BD_SDMC2",
    "BD_SPINOR", "BD_SPINAND", "BD_SDFAT32", "BD_USB", "BD_UDISK",
};

static void show_boot_device(u32 dev)
{
    static u32 show_flag = 1;
    const char *p;

    if (show_flag) {
        /* Print once only */
        if (dev < 8)
            p = boot_device_name[dev];
        else
            p = "BD_NONE";

        printf("Boot device = %u(%s)\n", dev, p);
        show_flag = 0;
    }
}

enum boot_device aic_get_boot_device(void)
{
    enum boot_device dev;

    /* SPL use a0 */
    dev = get_boot_device(boot_params_stash.r.a[0]);
    show_boot_device(dev);
    return dev;
}

enum boot_controller aic_get_boot_controller(void)
{
    return get_boot_controller(boot_params_stash.r.a[0]);
}

void *aic_get_boot_resource(void)
{
    return (void *)(boot_params_stash.r.a[1]);
}

void aic_set_boot_resource(void *res_addr)
{
    boot_params_stash.r.a[1] = (u32)(uintptr_t)res_addr;
}

int aic_get_boot_image_id(void)
{
    return get_boot_image_id(boot_params_stash.r.a[0]);
}

unsigned long aic_timer_get_us(void)
{
    register unsigned long tick;

    __asm volatile("csrr %0, 0xc01"
                   : "=r"(tick)::"memory"); /* 0xc01 is csr time. */

    return (tick >> 2);
}

void *aic_get_boot_args(void)
{
    return (void *)&boot_arg;
}
