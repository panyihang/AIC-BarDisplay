/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
#include <aic_common.h>

void show_speed(char *msg, unsigned long len, unsigned long us)
{
    unsigned long long tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = (unsigned long long)1000 * len;
    tmp = tmp / us;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    printf("%s: %ld byte, %ld us -> %lld KB/s\n", msg, len, us, speed);
}

