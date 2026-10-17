/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <image.h>
#include <boot.h>
#include <aic_core.h>
#include <aic_time.h>
#include <boot_time.h>
#include <console.h>
#include <boot_param.h>

void boot_app(void *app)
{
    void (*ep)(int, unsigned long);
    enum boot_device dev;
    unsigned long boot_arg;

#ifndef LPKG_USING_FDTLIB
    ep = image_get_entry_point(app);
#else
    ep = app;
#endif
    if (!ep) {
        printf("Entry point is null, Run APP failure.\n");
        return;
    }
    boot_time_trace("Run APP");
    boot_time_show();
    dev = aic_get_boot_device();
    boot_arg = (unsigned long)aic_get_boot_args();
    aicos_dcache_clean();
    aicos_icache_invalid();
    aicos_local_irq_disable();

    ep(dev, boot_arg);
}
