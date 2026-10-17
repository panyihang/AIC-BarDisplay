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

static int
clk_fixed_parent_enable_and_deassert_rst(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val;

    /* enbale clk */
    val = readl(cmu_reg(mod->offset_reg));

    if (mod->mod_gate_bit >= 0)
        val |= (1 << mod->mod_gate_bit);
    if (mod->bus_gate_bit >= 0)
        val |= (1 << mod->bus_gate_bit);
    writel(val, cmu_reg(mod->offset_reg));

    aicos_udelay(30);

    /* deassert rst */
    val = readl(cmu_reg(mod->offset_reg));
    val |= (1 << MOD_RSTN);

    writel(val, cmu_reg(mod->offset_reg));
    aicos_udelay(30);

    return 0;
}

static void
clk_fixed_parent_disable_and_assert_rst(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val;

    /* assert rst */
    val = readl(cmu_reg(mod->offset_reg));

    val &= ~(1 << MOD_RSTN);

    writel(val, cmu_reg(mod->offset_reg));

    aicos_udelay(30);

    /* disbale clk */
    val = readl(cmu_reg(mod->offset_reg));

    if (mod->mod_gate_bit >= 0)
        val &= ~(1 << mod->mod_gate_bit);
    if (mod->bus_gate_bit >= 0)
        val &= ~(1 << mod->bus_gate_bit);

    writel(val, cmu_reg(mod->offset_reg));

    aicos_udelay(30);
}

static int clk_fixed_parent_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val;

    val = readl(cmu_reg(mod->offset_reg));

    if (mod->mod_gate_bit >= 0)
        val |= (1 << mod->mod_gate_bit);
    if (mod->bus_gate_bit >= 0)
        val |= (1 << mod->bus_gate_bit);

    writel(val, cmu_reg(mod->offset_reg));

    return 0;
}

static void clk_fixed_parent_disable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val;

    val = readl(cmu_reg(mod->offset_reg));

    if (mod->mod_gate_bit >= 0)
        val &= ~(1 << mod->mod_gate_bit);
    if (mod->bus_gate_bit >= 0)
        val &= ~(1 << mod->bus_gate_bit);

    writel(val, cmu_reg(mod->offset_reg));
}

static int clk_fixed_parent_mod_is_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    u32 val, mod_gate, bus_gate;

    val = readl(cmu_reg(mod->offset_reg));
    if (mod->mod_gate_bit >= 0)
        mod_gate = val & (1 << mod->mod_gate_bit);
    else
        mod_gate = 1;
    if (mod->bus_gate_bit >= 0)
        bus_gate = val & (1 << mod->bus_gate_bit);
    else
        bus_gate = 1;

    if (mod_gate && bus_gate)
        return 1;

    return 0;
}

static unsigned long
clk_fixed_parent_mod_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                                 unsigned long parent_rate)
{
    struct aic_clk_fixed_parent_cfg *mod = to_clk_fixed_parent(comm_cfg);
    unsigned long rate, div;

    if (!mod->div_mask)
        return parent_rate;

    div  = (readl(cmu_reg(mod->offset_reg)) >> mod->div_bit) & mod->div_mask;
    div *= mod->div_step;
    div += mod->div_base;
    rate = parent_rate / div;
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
    if (!rate || !mod->div_mask)
        return parent_rate;

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
    u32 val, div;

    if (!mod->div_mask)
        return 0;

    div = (parent_rate + rate / 2) / rate;
    if (div > mod->div_base) {
        div -= mod->div_base;
        div += (mod->div_step / 2);
        div /= mod->div_step;
    } else {
        div = 0;
    }

    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(mod->div_mask << mod->div_bit);
    val |= (div << mod->div_bit);
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
