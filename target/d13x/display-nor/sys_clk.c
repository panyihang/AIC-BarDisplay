/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>
#include "board.h"

struct aic_sysclk {
    unsigned long freq;
    unsigned int clk_id;
    unsigned int parent_clk_id;
};

struct aic_sysclk aic_sysclk_config[] = {
    {AIC_CLK_PLL_INT0_FREQ, CLK_PLL_INT0, 0},
    {AIC_CLK_PLL_INT1_FREQ, CLK_PLL_INT1, 0},
    {AIC_CLK_PLL_FRA0_FREQ, CLK_PLL_FRA0, 0},
    {AIC_CLK_PLL_FRA2_FREQ, CLK_PLL_FRA2, 0},
    {AIC_CLK_CPU_FREQ, CLK_CPU, CLK_CPU_SRC1},
    {AIC_CLK_AXI0_FREQ, CLK_AXI0, CLK_AXI_AHB_SRC1},
    {AIC_CLK_AHB0_FREQ, CLK_AHB0, CLK_AXI_AHB_SRC1},
    {AIC_CLK_APB0_FREQ, CLK_APB0, CLK_APB0_SRC1},
};

static void usb_ehci_disable(void)
{
    hal_clk_disable_assertrst(CLK_USBH0);
    hal_clk_disable(CLK_USBH0);
}

void aic_board_sysclk_init(void)
{
    uint32_t i;

    usb_ehci_disable();
    for (i = 0; i < ARRAY_SIZE(aic_sysclk_config); i++) {
        if (!aic_sysclk_config[i].freq)
            continue;

        if (aic_sysclk_config[i].parent_clk_id) {
            hal_clk_set_freq(aic_sysclk_config[i].parent_clk_id,
                             aic_sysclk_config[i].freq);
            hal_clk_enable(aic_sysclk_config[i].parent_clk_id);
            hal_clk_set_parent(aic_sysclk_config[i].clk_id,
                               aic_sysclk_config[i].parent_clk_id);
        } else {
            hal_clk_set_freq(aic_sysclk_config[i].clk_id,
                             aic_sysclk_config[i].freq);
            hal_clk_enable(aic_sysclk_config[i].clk_id);
        }
    }

    hal_clk_enable_deassertrst_iter(CLK_GPIO);
    hal_clk_enable_deassertrst_iter(CLK_GTC);
}
