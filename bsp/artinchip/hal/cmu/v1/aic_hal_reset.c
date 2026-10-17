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

static void aic_reset_set(u32 rst_id, unsigned int assert)
{
    unsigned int value;

    value = readl(cmu_reg(aic_reset_signals[rst_id].offset));
    if (assert)
        value &= ~aic_reset_signals[rst_id].bit;
    else
        value |= aic_reset_signals[rst_id].bit;
    writel(value, cmu_reg(aic_reset_signals[rst_id].offset));
}

int hal_reset_assert(uint32_t rst_id)
{
    CHECK_PARAM(rst_id < RESET_NUMBER, -EINVAL);

    aic_reset_set(rst_id, 1);
    return 0;
}

int hal_reset_deassert(uint32_t rst_id)
{
    CHECK_PARAM(rst_id < RESET_NUMBER, -EINVAL);

    aic_reset_set(rst_id, 0);
    return 0;
}

int hal_reset_status(uint32_t rst_id)
{
    unsigned int value;

    CHECK_PARAM(rst_id < RESET_NUMBER, -EINVAL);

    value = readl(cmu_reg(aic_reset_signals[rst_id].offset));
    return !(value & aic_reset_signals[rst_id].bit);
}
