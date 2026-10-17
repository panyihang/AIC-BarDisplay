/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#define PLL_WAIT_TIMEOUT    10 //unit: ms
#define to_clk_pll(_hw) container_of(_hw, struct aic_clk_pll_cfg, comm)

#ifdef AIC_CMU_DRV_V32
static void pll_write_auth(uint32_t val, volatile void *reg_addr)
{
    uint32_t offset;
    u64 timeout = 0;

    offset = (uint32_t)reg_addr - CMU_BASE;
    writel((0xA1C << 20) | offset, cmu_reg(0xFE8));
    timeout = PLL_WAIT_TIMEOUT + aic_get_time_ms();
    while (!(readl(cmu_reg(0xFE8)) & (1 << 16))) {
        if (aic_get_time_ms() >= timeout) {
            hal_log_err("PLL get auth timeout\n");
            return;
        }
    }
    writel(val, reg_addr);
}
#endif

#ifndef AIC_CMU_DRV_V32
#define PLL_WRITE           writel
#else
#define PLL_WRITE           pll_write_auth
#endif

/* ALL chips:
 * Other vco of clock not change
 * The vco of pll_fra2 range from (768M-1560M) to (360M~1584M)
 */
static const struct aic_pll_vco vco_arr[] = {
    {360000000, 1584000000, "pll_fra2"},
    {768000000, 1560000000,    "other"},
};

#ifndef AIC_CMU_DRV_V30
static int clk_pll_wait_lock(void)
{
    aic_udelay(200);
    return 0;
}
#endif

static void clk_vco_select(struct aic_clk_pll_cfg *pll,
                           unsigned long *min, unsigned long *max)
{
    const struct aic_pll_vco *vco;

    for (int i = 0; i < (ARRAY_SIZE(vco_arr) - 1); i++) {
        vco = &vco_arr[i];
        if (pll->id == hal_clk_get_id(vco->name)) {
            *min = vco->vco_min;
            *max = vco->vco_max;
            return;
        }
    }
    *max = vco_arr[ARRAY_SIZE(vco_arr) - 1].vco_max;
    *min = vco_arr[ARRAY_SIZE(vco_arr) - 1].vco_min;
}

static inline void clk_pll_bypass(struct aic_clk_pll_cfg *pll, unsigned int bypass)
{
#ifndef AIC_CMU_DRV_V30
    u32 val;

    val = readl(cmu_reg(pll->offset_gen));
    val &= ~(1 << PLL_OUT_MUX);
    val |= (!bypass << PLL_OUT_MUX);
    PLL_WRITE(val, cmu_reg(pll->offset_gen));
#endif
}

static void clk_pll_enable_sdm(struct aic_clk_pll_cfg *pll)
{
    u32 reg_val, ppm_max, sdm_amp, factor_n;
    u64 sdm_step;

    factor_n = (readl(cmu_reg(pll->offset_gen)) >> PLL_FACTORN_BIT) &
               PLL_FACTORN_MASK;

    ppm_max = 1000000 / (factor_n + 1);
    /* 1% spread */
    if (ppm_max < PLL_SDM_SPREAD_PPM)
        sdm_amp = 0;
    else
        sdm_amp = PLL_SDM_AMP_MAX -
                  PLL_SDM_SPREAD_PPM * PLL_SDM_AMP_MAX / ppm_max;

    /* SDM uses triangular wave, 33KHz by default  */
    sdm_step = (u64)(PLL_SDM_AMP_MAX - sdm_amp) * 2 * PLL_SDM_SPREAD_FREQ;
    /* PLL parent clock freq is a fixed 24MHz */
    do_div(sdm_step, CLOCK_24M);
    if (sdm_step > 511)
        sdm_step = 511;

    reg_val = (1UL << PLL_SDM_EN_BIT) |
              (2 << PLL_SDM_MODE_BIT) |
              (sdm_step << PLL_SDM_STEP_BIT) |
              (3 << PLL_SDM_FREQ_BIT) |
              (sdm_amp << PLL_SDM_AMP_BIT);

    PLL_WRITE(reg_val, cmu_reg(pll->offset_sdm));
}

static int clk_pll_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 val, sdm_en, sdm_bot;
#ifdef AIC_CMU_DRV_V30
    u64 timeout = 0;
#endif

    if (pll->flag & CLK_NO_CHANGE) {
        return 0;
    }


    val = readl(cmu_reg(pll->offset_gen));
#ifndef AIC_CMU_DRV_V30
    val |= (1 << PLL_OUT_SYS | 1 << PLL_EN_BIT);
#else
    val |= 1 << PLL_EN_BIT;
    PLL_WRITE(val, cmu_reg(pll->offset_gen));
    PLL_WRITE(0x7, cmu_reg(pll->offset_gen + 0xC));
    timeout = PLL_WAIT_TIMEOUT + aic_get_time_ms();
    while (!(readl(cmu_reg(pll->offset_gen + 0xC)) & 0x10)) {
        if (aic_get_time_ms() >= timeout) {
            hal_log_err("wait PLL stable timeout\n");
            return -1;
        }
    }
    val |= 1 << PLL_OUT_SYS;
#endif
    PLL_WRITE(val, cmu_reg(pll->offset_gen));

#ifndef AIC_CMU_DRV_V30
    clk_pll_wait_lock();
#endif

    if (pll->type == AIC_PLL_SDM) {
        val = readl(cmu_reg(pll->offset_sdm));
        sdm_en = val >> 31;
        sdm_bot = val & 0xFFFF;

        /*
         * When the PLL is fractional PLL, SDM_EN will also be set to 1,
         * so SDM_BOT is needed to help determine whether the spreading function
         * has been enabled.
         */
        if (!sdm_en || !sdm_bot)
            clk_pll_enable_sdm(pll);
    }

    return 0;
}

static void clk_pll_disable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 val;

    if (!(pll->flag & CLK_NO_CHANGE)) {
        val = readl(cmu_reg(pll->offset_gen));
        val &= ~(1 << PLL_OUT_SYS | 1 << PLL_EN_BIT);
        PLL_WRITE(val, cmu_reg(pll->offset_gen));
    }
}

static int clk_pll_is_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 reg_val                 = readl(cmu_reg(pll->offset_gen));

    if ((reg_val & (1 << PLL_EN_BIT)) && (reg_val & (1 << PLL_OUT_SYS)))
        return 1;

    return 0;
}

static unsigned long clk_pll_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                                         unsigned long parent_rate)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 factor_n, factor_m, factor_p, fra_in;
    u64 rate, rate_int, rate_fra;
    u8 fra_en = 0;

#ifndef AIC_CMU_DRV_V30
    /* PLL output mux is CLK_24M */
    if (!((readl(cmu_reg(pll->offset_gen)) >> PLL_OUT_MUX) & 0x1))
        return CLOCK_24M;
#endif

    factor_n = (readl(cmu_reg(pll->offset_gen)) >> PLL_FACTORN_BIT) &
               PLL_FACTORN_MASK;
    factor_m = (readl(cmu_reg(pll->offset_gen)) >> PLL_FACTORM_BIT) &
               PLL_FACTORM_MASK;
    factor_p = (readl(cmu_reg(pll->offset_gen)) >> PLL_FACTORP_BIT) &
               PLL_FACTORP_MASK;
    if (pll->type == AIC_PLL_FRA)
        fra_en = (readl(cmu_reg(pll->offset_fra)) >> PLL_FRAC_EN_BIT) & 0x1;

    if (pll->type == AIC_PLL_INT || !fra_en) {
        rate = parent_rate / (factor_p + 1) * (factor_n + 1) / (factor_m + 1);
    } else {
        fra_in = readl(cmu_reg(pll->offset_fra)) & PLL_FRAC_DIV_MASK;
        rate_int =
                parent_rate / (factor_p + 1) * (factor_n + 1) / (factor_m + 1);
        rate_fra = (u64)parent_rate / (factor_p + 1) * fra_in;
        do_div(rate_fra, PLL_FRAC_DIV_MASK * (factor_m + 1));
        rate = rate_int + rate_fra;
    }

#ifdef FPGA_BOARD_ARTINCHIP
    rate = fpga_board_rate[pll->id];
#endif
    return rate;
}

static long clk_pll_round_rate(struct aic_clk_comm_cfg *comm_cfg,
                               unsigned long rate, unsigned long *prate)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 factor_n, factor_m, factor_p;
    long rrate, vco_rate;
    unsigned long pll_vco_min, pll_vco_max;
    unsigned long parent_rate = *prate;

    if (pll->type == AIC_PLL_FRA)
            return rate;

    clk_vco_select(pll, &pll_vco_min, &pll_vco_max);

    /* The frequency constraint of PLL_VCO is between 768M and 1560M
     * But the PLL_VCO of pll_fra2 is between 360M and 1584M
     */
    if (rate < pll_vco_min)
        factor_m = DIV_ROUND_UP(pll_vco_min, rate) - 1;
    else
        factor_m = 0;

    if (factor_m > PLL_FACTORM_MASK)
        factor_m = PLL_FACTORM_MASK;

    vco_rate = (factor_m + 1) * rate;
    if (vco_rate > pll_vco_max)
        vco_rate = pll_vco_max;

    factor_p = (vco_rate % parent_rate) ? 1 : 0;
    if (!factor_p)
        return rate;
    else if (!(vco_rate % (parent_rate / (factor_p + 1))))
        return rate;

    factor_n = vco_rate / parent_rate * (factor_p + 1) - 1;

    rrate = parent_rate / (factor_p + 1) * (factor_n + 1) / (factor_m + 1);

#ifdef FPGA_BOARD_ARTINCHIP
    rrate = fpga_board_rate[pll->id];
#endif
    return rrate;
}

static int clk_pll_set_rate(struct aic_clk_comm_cfg *comm_cfg,
                            unsigned long rate, unsigned long parent_rate)
{
    u32 factor_n, factor_m, factor_p, reg_val;
    u64 val, fra_in = 0;
    u8 fra_en, factor_m_en;
    u32 sdm_en;
    unsigned long vco_rate, pll_vco_min, pll_vco_max;
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);

    if (pll->flag & CLK_NO_CHANGE)
        return 0;

    clk_vco_select(pll, &pll_vco_min, &pll_vco_max);

#ifndef AIC_CMU_DRV_V30
    if (rate == CLOCK_24M) {
        val = readl(cmu_reg(pll->offset_gen));
        val &= ~(1 << PLL_OUT_MUX);
        PLL_WRITE(val, cmu_reg(pll->offset_gen));
        return 0;
    }
#endif

    /* Switch the output of PLL to 24MHz */
    clk_pll_bypass(pll, 1);

    /* Calculate PLL parameters.
     * The frequency constraint of PLL_VCO is between 768M and 1560M
     * But the PLL_VCO of pll_fra2 is between 360M and 1584M
     */
    if (rate < pll_vco_min)
        factor_m = DIV_ROUND_UP(pll_vco_min, rate) - 1;
    else
        factor_m = 0;

    /* Avoid factor_m == 2 */
    if (factor_m == 2)
        factor_m = 3;

    if (factor_m > PLL_FACTORM_MASK)
        factor_m = PLL_FACTORM_MASK;

    if (factor_m)
        factor_m_en = 1;
    else
        factor_m_en = 0;

    vco_rate = (factor_m + 1) * rate;
    if (vco_rate > pll_vco_max)
        vco_rate = pll_vco_max;

    factor_p = (vco_rate % parent_rate) ? 1 : 0;
    factor_n = vco_rate * (factor_p + 1) / parent_rate  - 1;

    reg_val = readl(cmu_reg(pll->offset_gen));
    reg_val &= ~0xFFFF;
    reg_val |= (factor_m_en << PLL_FACTORM_EN_BIT) |
            (factor_n << PLL_FACTORN_BIT) |
            (factor_m << PLL_FACTORM_BIT) |
            (factor_p << PLL_FACTORP_BIT);
    /* If SDM enable, set PLL_ICP = 0 */
    if (pll->type == AIC_PLL_SDM)
        reg_val &= ~(0x1F << 24);
    PLL_WRITE(reg_val, cmu_reg(pll->offset_gen));

    if (pll->type == AIC_PLL_FRA) {
        val = rate % (parent_rate * (factor_n + 1) /
                  (factor_m + 1) / (factor_p + 1));
        fra_en = val ? 1 : 0;
        if (fra_en) {
            fra_in = val * (factor_p + 1) *
                 (factor_m + 1) * PLL_FRAC_DIV_MASK;
            do_div(fra_in, parent_rate);
        }
        /* Configure fractional division */
        PLL_WRITE(fra_en << PLL_FRAC_EN_BIT | fra_in, cmu_reg(pll->offset_fra));
        /* when using decimal divsion, do not configure spreading parameters */
        sdm_en = (1UL << PLL_SDM_EN_BIT) | (2UL << PLL_SDM_MODE_BIT);
        PLL_WRITE(sdm_en, cmu_reg(pll->offset_sdm));
    }

#ifndef AIC_CMU_DRV_V30
    reg_val = readl(cmu_reg(pll->offset_gen));
    reg_val |= (1 << PLL_OUT_SYS | 1 << PLL_EN_BIT);
    PLL_WRITE(reg_val, cmu_reg(pll->offset_gen));

    if (!clk_pll_wait_lock()) {
        clk_pll_bypass(pll, 0);
    } else {
        //pr_err("%d not lock\n", pll->id);
        return -EAGAIN;
    }
#endif

    return 0;
}

static unsigned int clk_pll_get_parent(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);

    return pll->parent_id;
}

const struct aic_clk_ops aic_clk_pll_ops = {
    .enable      = clk_pll_enable,
    .disable     = clk_pll_disable,
    .is_enabled  = clk_pll_is_enable,
    .recalc_rate = clk_pll_recalc_rate,
    .round_rate  = clk_pll_round_rate,
    .set_rate    = clk_pll_set_rate,
    .get_parent  = clk_pll_get_parent,
};

void hal_clk_pll_lowpower(void)
{
#if defined(AIC_CMU_DRV_V10)
    #if 0
    *(volatile uint32_t *)(CMU_BASE+PLL_IN_REG) &= ~(0x7U << 29);
    #endif
#elif defined(AIC_CMU_DRV_V11)
    int xtal_en = readl(SID_BASE + 0x1C) & (0x1 << 1);
    if (xtal_en)
        *(volatile uint32_t *)(CMU_BASE+PLL_IN_REG) &= ~((0x7U << 29) | (0x1U << 1));
    else
        *(volatile uint32_t *)(CMU_BASE+PLL_IN_REG) &= ~((0x7U << 29) | (0x1U << 28));
#elif defined(AIC_CMU_DRV_V12)
    *(volatile uint32_t *)(CMU_BASE+PLL_IN_REG) &= ~((0x7U << 29) | (0x1U << 1));
#endif
}
