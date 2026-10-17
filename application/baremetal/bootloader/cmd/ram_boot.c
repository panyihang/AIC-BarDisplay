/*
 * Copyright (c) 2023-2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <image.h>
#include <boot.h>
#include "aic_time.h"
#ifdef CONFIG_LPKG_USING_FDTLIB
#include "fitimage.h"
#include "of.h"
#endif

#define APPLICATION_PART "os"

static int do_ram_boot(int argc, char *argv[])
{
    int ret = 0;
    uint8_t *data;
    struct image_header head;
    void *la;
    unsigned long addr;
#ifdef CONFIG_LPKG_USING_FDTLIB
    ulong entry_point;
    struct spl_load_info info;
#endif

    addr = strtoul(argv[1], NULL, 0);
    data = (uint8_t *)addr;
    memcpy(&head, data, sizeof(head));
    ret = image_verify_magic((void *)&head, AIC_IMAGE_MAGIC);
    if (ret) {
#ifdef CONFIG_LPKG_USING_FDTLIB
        /* fitimage format */
        info.dev = NULL;
        info.dev_type = DEVICE_RAM;
        info.bl_len = 1;
        info.priv = (void *)addr;

        entry_point = 0;
        ret = spl_load_simple_fit(&info, &entry_point);
        if (ret < 0)
            return ret;

        boot_app((void *)entry_point);
#else
        printf("Application header is unknown.\n");
        return -1;
#endif
    } else {
        /* AIC image format */
        la = (void *)(unsigned long)head.load_address;

        memcpy(la, data, head.image_len);

        if (ret < 0)
            return -1;

        boot_app(la);
    }

    return ret;
}

CONSOLE_CMD(ram_boot, do_ram_boot, "Boot from RAM.");
