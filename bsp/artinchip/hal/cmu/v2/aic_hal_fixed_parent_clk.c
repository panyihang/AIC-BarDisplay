/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#define to_clk_fixed_parent(_hw) \
    container_of(_hw, struct aic_clk_fixed_parent_cfg, comm)

static int clk_fixed_parent_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 i, val;

    if (!mod->table_gates) {
        hal_log_err("%s does not have valid table_gates\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->table_gates[0] < 0)
        return 0;

    val = readl(cmu_reg(mod->offset_reg));

    for (i = 0; i < mod->num_gates; i++) {
        val |= (1 << mod->table_gates[i]);
        writel(val, cmu_reg(mod->offset_reg));
    }

    return 0;
}

static void clk_fixed_parent_disable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val = 0;
    s32 i;

    if (!mod->table_gates) {
        hal_log_err("%s does not have valid table_gates\n", comm_cfg->name);
        return;
    }

    if (mod->table_gates[0] < 0)
        return;

    val = readl(cmu_reg(mod->offset_reg));

    for (i = mod->num_gates - 1; i >= 0; i--) {
        val &= ~(1 << mod->table_gates[i]);
        writel(val, cmu_reg(mod->offset_reg));
    }
}

static int clk_fixed_parent_enable_and_deassert_rst(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val;
    s32 ret;

    ret = clk_fixed_parent_enable(comm_cfg);
    if (ret)
        return ret;

    aicos_udelay(30);

    /* deassert rst */
    val = readl(cmu_reg(mod->offset_reg));
    val |= (1 << MOD_RSTN);
    writel(val, cmu_reg(mod->offset_reg));

    aicos_udelay(30);

    return 0;
}

static void clk_fixed_parent_disable_and_assert_rst(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val;

    /* assert rst */
    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(1 << MOD_RSTN);
    writel(val, cmu_reg(mod->offset_reg));

    aicos_udelay(30);

    clk_fixed_parent_disable(comm_cfg);

    aicos_udelay(30);
}

static int clk_fixed_parent_mod_is_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val, i, ret = 1;

    if (!mod->table_gates) {
        hal_log_err("%s does not have valid table_gates\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->table_gates[0] < 0)
        return ret;

    val = readl(cmu_reg(mod->offset_reg));

    for (i = 0;  i < mod->num_gates; i++) {
        ret &= ((val & (1 << mod->table_gates[i])) >> mod->table_gates[i]);
        if (!ret)
            return ret;
    }

    return ret;
}

static unsigned long clk_fixed_parent_mod_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                                                      unsigned long parent_rate)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    unsigned long rate, div, div_mask, val;

    if (!mod->table_div) {
        hal_log_err("%s does not have valid table_div\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->table_div[0].shift < 0) {
        rate = parent_rate / mod->table_div[0].wd.div;
    } else {
        div_mask = (1 << mod->table_div[0].wd.width) - 1;
        val = readl(cmu_reg(mod->offset_reg));
        div = (val >> mod->table_div[0].shift) & div_mask;
        rate = parent_rate / (div + 1);
    }

#ifdef FPGA_BOARD_ARTINCHIP
    rate = fpga_board_rate[mod->id];
#endif
    return rate;
}

static long clk_fixed_parent_mod_round_rate(struct aic_clk_comm_cfg *comm_cfg,
                                            unsigned long rate,
                                            unsigned long *prate)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    unsigned long rrate, parent_rate, div;

    parent_rate = *prate;
    if (!rate || !mod->table_div) {
        hal_log_err("%s does not have valid params\n", comm_cfg->name);
        return parent_rate;
    }

    div = parent_rate / rate;
    div += (parent_rate - rate * div) > (rate / 2) ? 1 : 0;
    if (div == 0)
        div = 1;

    rrate = parent_rate / div;
#ifdef FPGA_BOARD_ARTINCHIP
    rrate = fpga_board_rate[mod->id];
#endif
    return rrate;
}

static int clk_fixed_parent_mod_set_rate(struct aic_clk_comm_cfg *comm_cfg,
                                         unsigned long rate,
                                         unsigned long parent_rate)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val, div, div_mask, div_max;

    if (!mod->table_div) {
        hal_log_err("%s does not have valid table_div\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->table_div[0].shift < 0)
        return 0;

    div = (parent_rate + rate / 2) / rate;
    div_max = 1 << mod->table_div[0].wd.width;
    if (div > div_max)
        div = div_max;

    val = readl(cmu_reg(mod->offset_reg));
    div_mask = div_max - 1;
    val &= ~(div_mask << mod->table_div[0].shift);
    val |= ((div - 1) << mod->table_div[0].shift);
    writel(val, cmu_reg(mod->offset_reg));

    return 0;
}

static unsigned int clk_fixed_parent_mod_get_parent(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);

    return mod->parent_id;
}

const struct aic_clk_ops aic_clk_fixed_parent_ops = {
    .enable                  = clk_fixed_parent_enable,
    .disable                 = clk_fixed_parent_disable,
    .is_enabled              = clk_fixed_parent_mod_is_enable,
    .recalc_rate             = clk_fixed_parent_mod_recalc_rate,
    .round_rate              = clk_fixed_parent_mod_round_rate,
    .set_rate                = clk_fixed_parent_mod_set_rate,
    .get_parent              = clk_fixed_parent_mod_get_parent,
    .enable_clk_deassert_rst = clk_fixed_parent_enable_and_deassert_rst,
    .disable_clk_assert_rst  = clk_fixed_parent_disable_and_assert_rst,
};
