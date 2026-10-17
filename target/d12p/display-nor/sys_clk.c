/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */

#include <aic_core.h>
#include <aic_hal.h>
#include "board.h"

struct aic_sysclk
{
    unsigned long       freq;
    unsigned int        clk_id;
    unsigned int        parent_clk_id;
};

struct aic_sysclk aic_sysclk_config[] = {
    {AIC_CLK_PLL_INT0_FREQ, CLK_PLL_INT0, 0},           /* default: 480MHz */
    {AIC_CLK_PLL_INT1_FREQ, CLK_PLL_INT1, 0},           /* default: 1200MHz */
    /* PLL_FRA0 is configured by pbp */
    {AIC_CLK_PLL_FRA1_FREQ, CLK_PLL_FRA1, 0},           /* default: 396MHz */
    {AIC_CLK_PLL_FRA2_FREQ, CLK_PLL_FRA2, 0},           /* default: 1188MHz */
    {AIC_CLK_CPU_FREQ,      CLK_CPU,      CLK_PLL_INT0},           /* default: 480MHz */
#ifdef AIC_USING_CLK_OUT0
    {AIC_CLK_OUT0_FREQ, CLK_OUT0, 0},
#endif /* AIC_USING_CLK_OUT0 */
};

void aic_board_sysclk_init(void)
{
    uint32_t i = 0;
    unsigned long parent_rate;

    for (i=0; i<sizeof(aic_sysclk_config)/sizeof(struct aic_sysclk); i++) {
        if (aic_sysclk_config[i].freq == 0)
            continue;

        /* multi parent clk */
        if (aic_sysclk_config[i].parent_clk_id) {
            parent_rate = hal_clk_get_freq(aic_sysclk_config[i].parent_clk_id);
            hal_clk_set_rate(aic_sysclk_config[i].clk_id, aic_sysclk_config[i].freq, parent_rate);
            hal_clk_set_parent(aic_sysclk_config[i].clk_id, aic_sysclk_config[i].parent_clk_id);
        } else {
            hal_clk_set_freq(aic_sysclk_config[i].clk_id, aic_sysclk_config[i].freq);
            hal_clk_enable(aic_sysclk_config[i].clk_id);
        }
    }

    /* Enable sys clk */
    hal_clk_enable_deassertrst_iter(CLK_GPIO);
    hal_clk_enable_deassertrst_iter(CLK_GTC);
}

