/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"
#include "aic_hal_reset.h"

extern const struct aic_reset_signal aic_reset_signals[];
#ifndef AIC_IN_SUSPEND_ENV
int cmu_auth_lock(u32 auth, u32 key, u32 shift, u32 lock, u32 status, u32 who);
int cmu_auth_request(u32 auth, u32 key, u32 shift, u32 status, u32 who);
#endif
static int aic_reset_set(u32 rst_id, unsigned int assert)
{
    const struct aic_reset_signal *rst = &aic_reset_signals[rst_id];
    unsigned int val;

    val = readl(cmu_reg(rst->offset));
    if (assert)
        val &= ~rst->bit;
    else
        val |= rst->bit;
#ifndef AIC_IN_SUSPEND_ENV
    if (rst->flags & AIC_RESET_AUTH_REQUEST) {
        if (cmu_auth_request(0xFE8, 0xA1C, 20, BIT(16), rst->offset))
            return -1;
    }
#endif
    writel(val, cmu_reg(rst->offset));
#ifndef AIC_IN_SUSPEND_ENV
    if (rst->flags & AIC_RESET_AUTH_LOCK) {
        if (cmu_auth_lock(0xFE8, 0xD15, 20, 0xFEC, BIT(0), rst->offset))
            return -1;
    }
#endif
    return 0;
}

int hal_reset_assert(uint32_t rst_id)
{
    CHECK_PARAM(rst_id < RESET_NUMBER, -EINVAL);

    return aic_reset_set(rst_id, 1);
}

int hal_reset_deassert(uint32_t rst_id)
{
    CHECK_PARAM(rst_id < RESET_NUMBER, -EINVAL);

    return aic_reset_set(rst_id, 0);
}

int hal_reset_status(uint32_t rst_id)
{
    unsigned int value;

    CHECK_PARAM(rst_id < RESET_NUMBER, -EINVAL);

    value = readl(cmu_reg(aic_reset_signals[rst_id].offset));
    return !(value & aic_reset_signals[rst_id].bit);
}
