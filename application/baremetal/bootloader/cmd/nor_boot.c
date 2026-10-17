/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
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
#include <sfud.h>
#include <mtd.h>
#include <image.h>
#include <boot.h>
#include <aic_utils.h>
#include "aic_time.h"
#include "fitimage.h"
#include "of.h"

#define APPLICATION_PART "os"

#ifdef AIC_AB_SYSTEM_INTERFACE
#include <absystem.h>

char target[32] = { 0 };
#endif

static int do_nor_boot(int argc, char *argv[])
{
    char target[32] __attribute__((unused)) = { 0 };
    struct spl_load_info info;
    struct mtd_dev *mtd;
    ulong entry_point;
    int ret = 0;

    mtd_probe();

#ifdef AIC_AB_SYSTEM_INTERFACE
    ret = aic_ota_check();
    if (ret) {
        printf("Aic ota check error.\n");
    }

    ret = aic_get_os_to_startup(target);
    printf("Start-up from %s\n", target);
    if (ret) {
        printf("Aic get os fail, startup from %s default.\n", APPLICATION_PART);
        mtd = mtd_get_device(APPLICATION_PART);
    } else {
        mtd = mtd_get_device(target);
    }
#else
    mtd = mtd_get_device(APPLICATION_PART);
#endif
    if (!mtd) {
        printf("Failed to get application partition.\n");
        return -1;
    }

    info.dev = (void *)mtd;
    info.bl_len = 1;
    info.dev_type = DEVICE_SPINOR;

    entry_point = 0;
    ret = spl_load_simple_fit(&info, &entry_point);
    if (ret < 0)
        goto out;

    /* Read dtb from config partition */
    of_fdt_dt_init_bare_nornand();

    boot_app((void *)entry_point);

out:
    return ret;
}

CONSOLE_CMD(nor_boot, do_nor_boot, "Boot from NOR.");
