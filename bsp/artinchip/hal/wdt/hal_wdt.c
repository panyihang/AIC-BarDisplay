/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "hal_wdt.h"

#define WDT_REG_CTL             (WDT_BASE + 0x000)
#define WDT_REG_CNT             (WDT_BASE + 0x004)
#define WDT_REG_IRQ_EN          (WDT_BASE + 0x008)
#define WDT_REG_IRQ_STA         (WDT_BASE + 0x00C)
#define WDT_REG_CLR_THD(n)      (WDT_BASE + 0x040 + (n) * 0x10)
#define WDT_REG_IRQ_THD(n)      (WDT_BASE + 0x044 + (n) * 0x10)
#define WDT_REG_RST_THD(n)      (WDT_BASE + 0x048 + (n) * 0x10)
#define WDT_REG_OP              (WDT_BASE + 0x0E8)

#ifdef AIC_WDT_DRV_V11
#define WDT_REG_RST_SEL         (WDT_BASE + 0x020)
#endif

#define WDT_REG_VER             (WDT_BASE + 0xFFC)

#define WDT_WR_DIS_SHIFT        28
#define WDT_WR_DIS_MASK         GENMASK(29, 28)
#define WDT_CFG_ID_SHIFT        24
#define WDT_CFG_ID_MASK         GENMASK(27, 24)
#define WDT_DBG_CNT_CONTINUE_SHIFT  1
#define WDT_CNT_EN              BIT(0)
#define WDT_REG_WR_DIS          BIT(28)

#define WDT_OP_CNT_CLR_CMD0     0xA1C55555
#define WDT_OP_CNT_CLR_CMD1     0xA1CAAAAA
#define WDT_OP_CFG_SW_CMD0(n)   (0xA1C5A5A0 | (n))
#define WDT_OP_CFG_SW_CMD1(n)   (0xA1CA5A50 | (n))
#define WDT_OP_WR_EN_CMD0       0xA1C99999
#define WDT_OP_WR_EN_CMD1       0xA1C66666

#define WDT_IRQ_STA_TIMEOUT     BIT(0)

#define WDT_SEC_TO_CNT(n)       ((n) * 32000)
#define WDT_CNT_TO_SEC(n)       ((n) / 32000)

enum aic_wdt_wr_mode {
    WDT_WR_ENABLE = 0,
    WDT_WR_DISABLE = 1, // Only can write WDT_REG_OP
    WDT_WR_DISABLE_ALL = 3 // Only can reset
};

static wdt_callback_t g_wdt_callback = NULL;

void hal_wdt_op_clr(u32 thd)
{
    writel(WDT_OP_CNT_CLR_CMD0, WDT_REG_OP);
    writel(WDT_OP_CNT_CLR_CMD1, WDT_REG_OP);
}

void hal_wdt_clr_thd_set(u32 ch, struct aic_wdt *wdt)
{
    writel(WDT_SEC_TO_CNT(wdt->clr_thd), WDT_REG_CLR_THD(ch));
}

void hal_wdt_irq_thd_set(u32 ch, struct aic_wdt *wdt)
{
    writel(WDT_SEC_TO_CNT(wdt->irq_thd), WDT_REG_IRQ_THD(ch));
}

void hal_wdt_rst_thd_set(u32 ch, struct aic_wdt *wdt)
{
    writel(WDT_SEC_TO_CNT(wdt->rst_thd), WDT_REG_RST_THD(ch));
}

void hal_wdt_thd_get(u32 ch, struct aic_wdt *wdt)
{
    wdt->clr_thd =  WDT_CNT_TO_SEC(readl(WDT_REG_CLR_THD(ch)));
    wdt->irq_thd =  WDT_CNT_TO_SEC(readl(WDT_REG_IRQ_THD(ch)));
    wdt->rst_thd =  WDT_CNT_TO_SEC(readl(WDT_REG_RST_THD(ch)));
}

void hal_wdt_switch_chan(int chan)
{
    writel(WDT_OP_CFG_SW_CMD0(chan), WDT_REG_OP);
    writel(WDT_OP_CFG_SW_CMD1(chan), WDT_REG_OP);
}

s32 hal_wdt_is_running(void)
{
    u32 val = readl(WDT_REG_CTL);

    return val & WDT_CNT_EN;
}

static u32 aic_wdt_cur_id(void)
{
    u32 val = readl(WDT_REG_CTL);

    return (val & WDT_CFG_ID_MASK) >> WDT_CFG_ID_SHIFT;
}

u32 hal_wdt_remain(struct aic_wdt *wdt)
{
    u32 val = readl(WDT_REG_CNT);

    val = WDT_CNT_TO_SEC(val);
    return wdt->timeout - val;
}

void hal_wdt_reg_protect(u8 enable)
{
    u32 val = 0;
    if (enable) {
        val = readl(WDT_REG_CTL);
        writel(val | WDT_REG_WR_DIS, WDT_REG_CTL);
        /* wdt clock protect */
        val = 0;
        val = readl(CMU_BASE + CLK_WDT_REG);
        writel(val | (0x10000000), CMU_BASE + CLK_WDT_REG);
    } else {
        writel(WDT_OP_WR_EN_CMD0, WDT_REG_OP);
        writel(WDT_OP_WR_EN_CMD1, WDT_REG_OP);
    }
}

void hal_wdt_enable(u32 enable, u32 dbg_continue)
{
    u32 val = 0;

    if (enable) {
        writel(WDT_CNT_EN
            | (dbg_continue << WDT_DBG_CNT_CONTINUE_SHIFT),
            WDT_REG_CTL);
        return;
    }

    val = readl(WDT_REG_CTL);
    val &= ~WDT_CNT_EN;
    writel(val, WDT_REG_CTL);
}

void hal_wdt_irq_enable(u32 enable)
{
    if (enable)
        writel(WDT_IRQ_STA_TIMEOUT, WDT_REG_IRQ_EN);
    else
        writel(0, WDT_REG_IRQ_EN);
}

int hal_wdt_irq_sta(void)
{
    return readl(WDT_REG_IRQ_EN);
}

int hal_wdt_clr_int(void)
{
    int sta = readl(WDT_REG_IRQ_STA);

    writel(sta, WDT_REG_IRQ_STA);
    return sta;
}

irqreturn_t hal_wdt_irq(int irq, void *arg)
{
    u8 val = 0;

    val = hal_wdt_clr_int();
    pr_debug("IRQ status %#x\n", val);

    if (val & WDT_IRQ_STA_TIMEOUT) {
        if (g_wdt_callback)
            g_wdt_callback();
    }

    return IRQ_HANDLED;
}

void hal_wdt_register_callback(wdt_callback_t callback)
{
    g_wdt_callback = callback;
}

#ifdef AIC_WDT_DRV_V11
void hal_wdt_rst_type_set(u32 rst)
{
    writel(rst, WDT_REG_RST_SEL);
}

int hal_wdt_rst_type_get(void)
{
    int sta = readl(WDT_REG_RST_SEL);
    return sta;
}
#endif

void hal_wdt_status_show(u32 ch)
{
    int ver = readl(WDT_REG_VER);

    printf("In Watchdog V%d.%02d:\n"
           "Module Enable: %d\n"
           "Write disable: %d\n"
           "IRQ Enable: %d\n"
           "Current chan: hw %d, sw %d\n"
           #ifdef AIC_WDT_DRV_V11
           "Current cnt: %d Reset object:%d\n"
           #else
           "Current cnt: %d\n"
           #endif
           "chan clr_thd irq_thd rst_thd\n"
           "   0 %7d %7d %7d\n",
           ver >> 8, ver & 0xFF, hal_wdt_is_running(),
           readl(WDT_REG_CTL) >> WDT_WR_DIS_SHIFT,
           readl(WDT_REG_IRQ_EN),
           aic_wdt_cur_id(), ch,
           readl(WDT_REG_CNT),
           #ifdef AIC_WDT_DRV_V11
           readl(WDT_REG_RST_SEL),
           #endif
           readl(WDT_REG_CLR_THD(0)),
           readl(WDT_REG_IRQ_THD(0)),
           readl(WDT_REG_RST_THD(0)));
}
