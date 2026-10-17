/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
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
#include <console.h>

#define MAX_RECORD_NUM 64

struct boot_time_rec {
    u32 time_us;
    char *msg;
};

static struct boot_time_rec bt_list[MAX_RECORD_NUM];
static int bt_index;

void boot_time_trace(char *msg)
{
    if (bt_index >= MAX_RECORD_NUM)
        return;
    bt_list[bt_index].time_us = aic_get_time_us();
    bt_list[bt_index].msg = msg;
    bt_index++;
}

void boot_time_show(void)
{
    int i, start;

    start = 0;
#ifdef AIC_SHOW_BOOT_TIME
    printf("Boot time:\n");
#else
    start = bt_index > 0 ? bt_index - 1 : 0;
#endif
    for (i = start; i < bt_index; i++) {
        printf(" %d\t: %s\n", bt_list[i].time_us, bt_list[i].msg);
    }
}
