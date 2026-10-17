/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji Chen <jiji.chen@artinchip.com>
 */

#include <rtconfig.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <aic_core.h>
#include <hal_qspi.h>
#include "qspi_internal.h"

void qspi_reg_dump(u32 base)
{
    u32 *p, i;

#ifdef QEMU_RUN
    return;
#endif

    p = (void *)(unsigned long)base;
    for (i = 0; i < 40; i++) {
        if (i % 4 == 0)
            printf("\n0x%lX : ", (unsigned long)p);
        printf("%08X ", *p);
        p++;
    }
    printf("\n");
}

void show_freq(char *msg, u32 id, u32 hz)
{
    printf("qspi%d %s: %dHz\n", id, msg, hz);
}

u32 qspi_calc_timeout(u32 bus_hz, u32 len)
{
    u32 tmo_cnt, tmo_us;
    u32 tmo_speed = 100;

#ifdef QEMU_RUN
    return 1;
#endif

    if (bus_hz < HAL_QSPI_MIN_FREQ_HZ)
        tmo_us = (1000000 * len * 8) / bus_hz;
    else if (bus_hz < 1000000)
        tmo_us = (1000 * len * 8) / (bus_hz / 1000);
    else
        tmo_us = (len * 8) / (bus_hz / 1000000);

    /* Add 100ms time padding */
    tmo_us += 100000;
    tmo_cnt = tmo_us / HAL_QSPI_WAIT_PER_CYCLE;

    /* Consider the speed limit of DMA or CPU copy.
     */
    if (len >= QSPI_TRANSFER_DATA_LEN_1M)
        tmo_speed = ((len / QSPI_CPU_DMA_MIN_SPEED_MS) + 1) * 1000;

    return max(tmo_cnt, tmo_speed);
}

u32 qspi_master_get_best_div_param(u32 sclk, u32 bus_hz, u32 *div)
{
    u32 cdr1_clk, cdr2_clk;
    int cdr2, cdr1;

    /* Get the best cdr1 param if going to use cdr1 */
    cdr1 = 0;
    while ((sclk >> cdr1) > bus_hz)
        cdr1++;
    if (cdr1 > 0xF)
        cdr1 = 0xF;

    /* Get the best cdr2 param if going to use cdr2 */
    cdr2 = (int)(sclk / (bus_hz * 2)) - 1;
    if (cdr2 < 0)
        cdr2 = 0;
    if (cdr2 > 0xFF)
        cdr2 = 0xFF;
    cdr2_clk = sclk / (2 * (cdr2 + 1));
    while (sclk / (2 * (cdr2 + 1)) > bus_hz) {
        cdr2++;
        if (cdr2 > 0xFF)
            break;
    }

    cdr1_clk = sclk >> cdr1;
    cdr2_clk = sclk / (2 * (cdr2 + 1));

    /* cdr1 param vs cdr2 param, use the best */
    if (cdr1_clk == bus_hz) {
        *div = cdr1;
        return 0;
    } else if (cdr2_clk == bus_hz) {
        *div = cdr2;
        return 1;
    } else if ((cdr2_clk < bus_hz) && (cdr1_clk < bus_hz)) {
        /* Two clks less than expect clk, use the larger one */
        if (cdr2_clk > cdr1_clk) {
            *div = cdr2;
            return 1;
        }
        *div = cdr1;
        return 0;
    }
    /*
     * 1. Two clks great than expect clk, use least one
     * 2. There is one clk less than expect clk, use it
     */
    if (cdr2_clk < cdr1_clk) {
        *div = cdr2;
        return 1;
    }
    *div = cdr1;
    return 0;
}

