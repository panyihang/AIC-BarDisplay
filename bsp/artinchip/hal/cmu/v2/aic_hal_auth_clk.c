/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#define to_clk_auth(_hw)                     \
    container_of(_hw, struct aic_clk_auth_cfg, comm)

int cmu_auth_lock(u32 auth, u32 key, u32 shift, u32 lock, u32 status, u32 who)
{
    u32 val = (key << shift) | who;

    writel(val, cmu_reg(auth));
    aicos_udelay(100);

    val = readl(cmu_reg(lock));
    if (!(val & status)) {
        hal_log_err("Failed to request authorize %X for 0x%x!\n", key >> 8, who);
        return -EIO;
    }
    return 0;
}

int cmu_auth_request(u32 auth, u32 key, u32 shift, u32 status, u32 who)
{
    return cmu_auth_lock(auth, key, shift, auth, status, who);
}

static int clk_auth_write(struct aic_clk_auth_cfg *mod, u32 val)
{
    if (!mod->offset_wr_auth_reg || (mod->wr_auth_bit < 0) || mod->key_bit < 0) {
        hal_log_err("%s params error!\n", mod->comm.name);
        return -EINVAL;
    }

    if (cmu_auth_request(mod->offset_wr_auth_reg,
                         mod->key_code, mod->key_bit, BIT(mod->wr_auth_bit),
                         mod->offset_reg))
        return -EIO;

    writel(val, cmu_reg(mod->offset_reg));

    return 0;
}

static int clk_auth_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);
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
        clk_auth_write(mod, val);
    }

    return 0;
}

static void clk_auth_disable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);
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
        clk_auth_write(mod, val);
    }
}

static int clk_auth_enable_and_deassert_rst(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);
    u32 val;

    clk_auth_enable(comm_cfg);

    aicos_udelay(30);

    /* deassert rst */
    val = readl(cmu_reg(mod->offset_reg));
    val |= (1 << MOD_RSTN);
    clk_auth_write(mod, val);

    aicos_udelay(30);

    return 0;
}

static void clk_auth_disable_and_assert_rst(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);
    u32 val;

    /* assert rst */
    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(1 << MOD_RSTN);
    clk_auth_write(mod, val);

    aicos_udelay(30);

    clk_auth_disable(comm_cfg);

    aicos_udelay(30);
}

static int clk_auth_mod_is_enabled(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);
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

static unsigned long clk_auth_mod_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                                                unsigned long parent_rate)
{
    unsigned long rate, val, div = 0, div_mask, parent_index = 0;
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);

    if (!mod->table_div) {
        hal_log_err("%s does not have valid table_div\n", comm_cfg->name);
        return parent_rate;
    }

    parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index >= mod->num_parents) {
        hal_log_err("%s parent clock index error!\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->num_parents != mod->num_div) {
        hal_log_err("%s parent number is not equal to divider number!\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->table_div[parent_index].shift < 0) {
        rate = parent_rate / mod->table_div[parent_index].wd.div;
    } else {
        div_mask = (1 << mod->table_div[parent_index].wd.width) - 1;
        val = readl(cmu_reg(mod->offset_reg));
        div = (val >> mod->table_div[parent_index].shift) & div_mask;
        rate = parent_rate / (div + 1);
    }

#ifdef CONFIG_DEBUG_ON_FPGA_BOARD_ARTINCHIP
    rate = fpga_board_rate[mod->id];
#endif
    return rate;
}

static unsigned int clk_auth_mod_get_parent(struct aic_clk_comm_cfg *comm_cfg)
{
struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);
    u32 parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index >= mod->num_parents) {
        hal_log_err("%s parent clock index %d error!\n",
                    comm_cfg->name, parent_index);
        return -EINVAL;
    }

    if (mod->num_parents != mod->num_div) {
        hal_log_err("%s parent number %d is not equal to divider number %d!\n",
                    comm_cfg->name, mod->num_parents, mod->num_div);
        return -EINVAL;
    }

    return mod->parent_ids[parent_index];
}

static int clk_auth_mod_set_parent(struct aic_clk_comm_cfg *comm_cfg,
                                    unsigned int parent_id)
{
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);
    u32 i, val, parent_index = 0xFFFF;

    for (i = 0; i < mod->num_parents; i++) {
        if (parent_id == mod->parent_ids[i]) {
            parent_index = i;
            break;
        }
    }

    if (parent_index == 0xFFFF) {
        return -EINVAL;
    }

    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(mod->mux_mask << mod->mux_bit);
    val |= parent_index << mod->mux_bit;
    clk_auth_write(mod, val);

    return 0;
}

static int clk_auth_mod_set_rate(struct aic_clk_comm_cfg *comm_cfg,
                                unsigned long rate,
                                unsigned long parent_rate)
{
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);
    u32 val, parent_index = 0;
    u32 div = 0, div_mask;

    if (!mod->table_div) {
        hal_log_err("%s does not have valid table_div\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->num_parents != mod->num_div) {
        hal_log_err("%s parent number is not equal to divider number!\n", comm_cfg->name);
        return -EINVAL;
    }

    div = DIV_ROUND_CLOSEST(parent_rate, rate);

    /*
     * Find the parent_index that can configure the divider.
     */
    for (parent_index = 0; parent_index < mod->num_parents; parent_index++)
        if (mod->table_div[parent_index].shift < 0)
            continue;
        else
            break;

    if (parent_index >= mod->num_parents) {
        hal_log_err("%s parent clock index error!\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->table_div[parent_index].shift < 0)
        return 0;

    div_mask = (1 << mod->table_div[parent_index].wd.width) - 1;

    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(div_mask << mod->table_div[parent_index].shift);
    val |= (div - 1) << mod->table_div[parent_index].shift;
    clk_auth_write(mod, val);

    return 0;
}

static long clk_auth_mod_round_rate(struct aic_clk_comm_cfg *comm_cfg,
                                    unsigned long rate,
                                    unsigned long *prate)
{
    u32 rrate, parent_rate, parent_index, div = 0;
    struct aic_clk_auth_cfg *mod = to_clk_auth(comm_cfg);

    if (!mod->table_div) {
        hal_log_err("%s does not have valid table_div\n", comm_cfg->name);
        return *prate;
    }

    parent_rate = *prate;
    parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index >= mod->num_parents) {
        hal_log_err("%s parent clock index error!\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->num_parents != mod->num_div) {
        hal_log_err("%s parent number is not equal to divider number!\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->table_div[parent_index].shift < 0) {
        rrate = parent_rate / mod->table_div[parent_index].wd.div;
    } else {
        div = DIV_ROUND_CLOSEST(parent_rate, rate);
        rrate = parent_rate / div;
    }

#ifdef FPGA_BOARD_ARTINCHIP
    rrate = fpga_board_rate[mod->id];
#endif
    return rrate;
}


const struct aic_clk_ops aic_clk_auth_ops = {
    .enable      = clk_auth_enable,
    .disable     = clk_auth_disable,
    .enable_clk_deassert_rst = clk_auth_enable_and_deassert_rst,
    .disable_clk_assert_rst  = clk_auth_disable_and_assert_rst,
    .is_enabled  = clk_auth_mod_is_enabled,
    .recalc_rate = clk_auth_mod_recalc_rate,
    .round_rate  = clk_auth_mod_round_rate,
    .get_parent  = clk_auth_mod_get_parent,
    .set_parent  = clk_auth_mod_set_parent,
    .set_rate    = clk_auth_mod_set_rate,
};
