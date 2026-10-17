/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#define to_clk_fixed_rate(_hw) \
    container_of(_hw, struct aic_clk_fixed_rate_cfg, comm)

static unsigned long
clk_fixed_rate_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                           unsigned long parent_rate)
{
    struct aic_clk_fixed_rate_cfg *mod = to_clk_fixed_rate(comm_cfg);

    return mod->rate;
}

const struct aic_clk_ops aic_clk_fixed_rate_ops = {
    .recalc_rate = clk_fixed_rate_recalc_rate,
};
