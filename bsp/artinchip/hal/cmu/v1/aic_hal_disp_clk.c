/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#define to_clk_disp_mod(_hw) container_of(_hw, struct aic_clk_disp_cfg, comm)

static unsigned long clk_disp_mod_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                                              unsigned long parent_rate)
{
    struct aic_clk_disp_cfg *mod = to_clk_disp_mod(comm_cfg);
    unsigned long rate, pix_divsel, divn, divm, divl;
    u32 reg_val;

    if (mod->divn_mask) {
        /* For sclk */
        divn = (readl(cmu_reg(mod->offset_reg)) >> mod->divn_bit) &
               mod->divn_mask;
        rate = parent_rate / (1 << divn);
    } else if (mod->divm_mask) {
        /* For pixclk */
        reg_val    = readl(cmu_reg(mod->offset_reg));
        pix_divsel = (reg_val >> mod->pix_divsel_bit) & mod->pix_divsel_mask;
        switch (pix_divsel) {
        case 0:
            divl = (reg_val >> mod->divl_bit) & mod->divl_mask;
            divm = (reg_val >> mod->divm_bit) & mod->divm_mask;
            rate = parent_rate / (1 << divl) / (divm + 1);
            break;
        case 1:
            rate = parent_rate * 2 / 7;
            break;
        case 2:
            rate = parent_rate * 2 / 9;
            break;
        default:
            return -EINVAL;
        }
    } else {
        return -EINVAL;
    }


#ifdef FPGA_BOARD_ARTINCHIP
    rate = fpga_board_rate[mod->id];
#endif
    return rate;
}

static void clk_disp_try_best_divider(u32 rate, u32 parent_rate, u32 max_divn,
                                      u32 *divn)
{
    u32 tmp, i, min_delta = U32_MAX;
    u32 best_divn = 0;

    for (i = 0; i <= max_divn; i++) {
        tmp = (1 << i) * rate;
        if (parent_rate == tmp) {
            best_divn = i;
            goto __out;
        }

        if (abs(parent_rate - tmp) < min_delta) {
            min_delta = abs(parent_rate - tmp);
            best_divn = i;
        }
    }

__out:
    *divn = best_divn;
}

static void clk_disp_try_best_divider_pixclk(u32 rate, u32 parent_rate,
                                             u32 divm_max, u32 *divm,
                                             u32 divl_max, u32 *divl,
                                             u8 *pix_divsel)
{
    u32 tmp, tmpm, tmpl, val0, val1, val2, i;

    /* Calculate clock division */
    tmp = DIV_ROUND_CLOSEST(parent_rate, rate);
    /* Calculate the value of divl */
    tmpl = DIV_ROUND_UP(tmp, divm_max);
    for (i = 0; i < divl_max; i++)
        if (1 << i >= tmpl)
            break;
    tmpm = tmp / (1 << i);

    if ((1 << i) * tmpm * rate == parent_rate) {
        *pix_divsel = 0;
    } else {
        val0 = abs((1 << i) * tmpm * rate - parent_rate);
        val1 = abs(rate * 7 / 2 - parent_rate);
        val2 = abs(rate * 9 / 2 - parent_rate);

        if (val0 < val1 && val0 < val2)
            *pix_divsel = 0;
        else if (val1 < val0 && val1 < val2)
            *pix_divsel = 1;
        else if (val2 < val0 && val2 < val1)
            *pix_divsel = 2;
        else
            *pix_divsel = 0;
    }

    *divm = tmpm - 1;
    *divl = i;
}

static long clk_disp_mod_round_rate(struct aic_clk_comm_cfg *comm_cfg,
                                    unsigned long rate, unsigned long *prate)
{
    struct aic_clk_disp_cfg *mod = to_clk_disp_mod(comm_cfg);
    u32 parent_rate, divn, divm, divl, rrate;
    u8 pix_divsel = 0;

    parent_rate = *prate;

    if (mod->divm_mask) {
        /* For pixclk */
        clk_disp_try_best_divider_pixclk(rate, parent_rate, mod->divm_mask + 1,
                                         &divm, mod->divl_mask + 1, &divl,
                                         &pix_divsel);
        if (pix_divsel == 1)
            rrate = parent_rate * 2 / 7;
        else if (pix_divsel == 2)
            rrate = parent_rate * 2 / 9;
        else
            rrate = parent_rate / (divm + 1) / (1 << divl);
    } else if (mod->divn_mask) {
        /* For sclk */
        clk_disp_try_best_divider(rate, parent_rate, mod->divn_mask, &divn);
        rrate = parent_rate / (1 << divn);
    } else {
        return -EINVAL;
    }


#ifdef FPGA_BOARD_ARTINCHIP
    rrate = fpga_board_rate[mod->id];
#endif
    return rrate;
}

static int clk_disp_mod_set_rate(struct aic_clk_comm_cfg *comm_cfg,
                                 unsigned long rate, unsigned long parent_rate)
{
    struct aic_clk_disp_cfg *mod = to_clk_disp_mod(comm_cfg);
    u32 divn, divm, divl, val;
    u8 pix_divsel = 0;

    val = readl(cmu_reg(mod->offset_reg));

    if (mod->divm_mask) {
        /* For pixclk */
        clk_disp_try_best_divider_pixclk(rate, parent_rate, mod->divm_mask + 1,
                                         &divm, mod->divl_mask + 1, &divl,
                                         &pix_divsel);
        if (pix_divsel) {
            val &= ~(mod->pix_divsel_mask << mod->pix_divsel_bit);
            val |= (pix_divsel << mod->pix_divsel_bit);
        } else {
            val &= ~((mod->pix_divsel_mask << mod->pix_divsel_bit) |
                     (mod->divm_mask << mod->divm_bit) |
                     (mod->divl_mask << mod->divl_bit));
            val |= (divm << mod->divm_bit) | (divl << mod->divl_bit);
        }

        writel(val, cmu_reg(mod->offset_reg));
    } else if (mod->divn_mask) {
        /* For sclk */
        clk_disp_try_best_divider(rate, parent_rate, mod->divn_mask, &divn);
        val &= ~(mod->divn_mask << mod->divn_bit);
        val |= (divn << mod->divn_bit);

        writel(val, cmu_reg(mod->offset_reg));
    }

    return 0;
}

static unsigned int clk_disp_mod_get_parent(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_disp_cfg *mod = to_clk_disp_mod(comm_cfg);

    return mod->parent_id;
}

const struct aic_clk_ops aic_clk_disp_ops = {
    .get_parent  = clk_disp_mod_get_parent,
    .recalc_rate = clk_disp_mod_recalc_rate,
    .round_rate  = clk_disp_mod_round_rate,
    .set_rate    = clk_disp_mod_set_rate,
};
