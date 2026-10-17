/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <wdt.h>

static int do_reset_boot(int argc, char *argv[])
{
    wdt_init();
    printf("Going to reboot ...\n");
    aic_set_reboot_reason(REBOOT_REASON_CMD_REBOOT);
    wdt_expire_now();
    while(1) {};
    return 0;
}

CONSOLE_CMD(reset, do_reset_boot,  "Reboot device.");
CONSOLE_CMD(reboot, do_reset_boot, "Reboot device.");
