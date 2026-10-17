/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_hal_dsi.h>
#include <mipi_display.h>

#include "drv_fb.h"

#undef pr_debug
#ifdef AIC_DISP_MIPI_DSI_DEBUG
#define pr_debug    pr_info
#else
#define pr_debug(fmt, ...)
#endif

struct aic_dsi_comp
{
    void *regs;
    int vc_num;
    u32 ln_assign;
    u32 ln_polrs;
    bool dc_inv;

    u32 pixclk;
    u64 sclk_rate;
    u64 pll_disp_rate;

    struct aic_panel *panel;
    struct panel_dsi *dsi;
    const struct display_timing *timing;
};
static struct aic_dsi_comp *g_aic_dsi_comp;

#ifdef AIC_DISP_PQ_TOOL
AIC_PQ_TOOL_PINMUX_CONFIG(disp_pinmux_config);
AIC_PQ_TOOL_SET_DISP_PINMUX_FOPS(disp_pinmux_config);
#endif

static struct aic_dsi_comp *aic_dsi_request_drvdata(void)
{
    return g_aic_dsi_comp;
}

static void aic_dsi_release_drvdata(void)
{

}

struct dsi_irq_info {
    u32 bit_mask;
    char *err_str;
};

static struct dsi_irq_info irq1_info[] = {
    { DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRCONTENTION_LP1, "PHY ERROR: Data0 ErrContention LP1" },
    { DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRCONTENTION_LP0, "PHY ERROR: Data0 ErrContention LP0" },
    { DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRCONTROL, "PHY ERROR:DSI Data0 ErrControl" },
    { DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRSYNCESC_LPDT, "PHY ERROR: Data0 ErrSyncEsc LPDT" },
    { DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRESC_ESCAPE_ENTRY, "PHY ERROR: Data0 ErrEsc escape entry" },
    { DSI_IRQ_STA1_ACK_ERR_STA_DSI_PROTOCOL_VIOLATION, "ACK ERROR: DSI Protocol Violation" },
    { DSI_IRQ_STA1_ACK_ERR_STA_INVALID_TRANSMISSION_LENGTH, "ACK ERROR: Invalid Transmission Length" },
    { DSI_IRQ_STA1_ACK_ERR_STA_DSI_VC_ID_INVALID, "ACK ERROR: DSI VC ID Invalid" },
    { DSI_IRQ_STA1_ACK_ERR_STA_DSI_DATA_TYPE_NOT_RECOGNIZED, "ACK ERROR: DSI Data Type Not Recognized" },
    { DSI_IRQ_STA1_ACK_ERR_STA_LONG_PACKET_CHECKSUM_ERROR, "ACK ERROR: Long Packet Checksum Error" },
    { DSI_IRQ_STA1_ACK_ERR_STA_ECC_ERROR_MULTI_BIT, "ACK ERROR: ECC Error multi-bit" },
    { DSI_IRQ_STA1_ACK_ERR_STA_ECC_ERROR_SINGLE_BIT, "ACK ERROR: ECC Error single-bit" },
    { DSI_IRQ_STA1_ACK_ERR_STA_CONTENTION_DETECTED, "ACK ERROR: Contention Detected" },
    { DSI_IRQ_STA1_ACK_ERR_STA_FALSE_CONTROL_ERROR, "ACK ERROR: False Control Error" },
    { DSI_IRQ_STA1_ACK_ERR_STA_PERIPHERAL_TIMEOUT_ERROR, "ACK ERROR: Peripheral Timeout Error" },
    { DSI_IRQ_STA1_ACK_ERR_STA_LOW_POWER_TRANSMIT_SYNC_ERROR, "ACK ERROR: Low Power Transmit Sync Error" },
    { DSI_IRQ_STA1_ACK_ERR_STA_ESCAPE_MODE_ENTRY_COMMAND_ERROR, "ACK ERROR: Escape Mode Entry Command Error" },
    { DSI_IRQ_STA1_ACK_ERR_STA_EOT_SYNC_ERROR, "ACK ERROR: EoT Sync Error" },
    { DSI_IRQ_STA1_ACK_ERR_STA_SOT_SYNC_ERROR, "ACK ERROR: SoT Sync Error" },
    { DSI_IRQ_STA1_ACK_ERR_STA_SOT_ERROR, "ACK ERROR: SoT Error" },
};

static struct dsi_irq_info irq2_info[] = {
    { DSI_IRQ_STA2_GEN_PLDRD_OV_STA,   "GEN PLD READ FIFO OVERFLOW" },
    { DSI_IRQ_STA2_GEN_PLDRD_UD_STA,   "GEN PLD READ FIFO UNDERFLOW" },
    { DSI_IRQ_STA2_GEN_PLDSEND_UD_STA, "GEN PLD SEND FIFO UNDERFLOW" },
    { DSI_IRQ_STA2_GEN_PLD_OV_STA,     "GEN PLD FIFO UNDERFLOW" },
    { DSI_IRQ_STA2_GEN_CMD_OV_STA,     "GEN CMD FIFO OVERFLOW" },
    { DSI_IRQ_STA2_DPI_FIFO_OV_STA,    "DPI FIFO OVERFLOW" },
    { DSI_IRQ_STA2_EOPT_ERR_STA,       "NOT RECEIVED EOPT" },
    { DSI_IRQ_STA2_PSIZE_ERR_STA,      "RECEIVE PACKET SIZE ERROR" },
    { DSI_IRQ_STA2_CRC_ERR_STA,        "RECEIVE PACKET CRC ERROR" },
    { DSI_IRQ_STA2_ECC_MERR_STA,       "RECEIVE PACKET CRC MULTT BIT ERROR" },
    { DSI_IRQ_STA2_ECC_SERR_STA,       "RECEIVE PACKET CRC SINGLE BIT ERROR" },
    { DSI_IRQ_STA2_TO_LPRX_STA,        "TO LP RX TIMEOUT" },
    { DSI_IRQ_STA2_TO_HSTX_STA,        "TO HS TX TIMEOUT" },
};

static irqreturn_t aic_dsi_handler(int irq, void *ctx)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();
    u32 irq1_status, irq2_status;
    u32 i;

    irq1_status = reg_read(comp->regs + DSI_IRQ_STA1);
    reg_write(comp->regs + DSI_IRQ_STA1, irq1_status);

    irq2_status = reg_read(comp->regs + DSI_IRQ_STA2);
    reg_write(comp->regs + DSI_IRQ_STA2, irq2_status);

    for (i = 0; i < ARRAY_SIZE(irq1_info); i++)
        if (irq1_info[i].bit_mask & irq1_status)
            pr_debug("%s\n",irq1_info[i].err_str);

    for (i = 0; i < ARRAY_SIZE(irq2_info); i++)
        if (irq2_info[i].bit_mask & irq2_status)
            pr_debug("%s\n",irq2_info[i].err_str);

    aic_dsi_release_drvdata();
    return IRQ_HANDLED;
}

static int aic_dsi_clk_enable(void)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();
    u32 pixclk = comp->pixclk;

    hal_clk_set_freq(CLK_PLL_FRA2, comp->pll_disp_rate);
    hal_clk_set_freq(CLK_SCLK, comp->sclk_rate);
    hal_clk_set_freq(CLK_PIX, pixclk);

    hal_clk_enable(CLK_PLL_FRA2);
    hal_clk_enable(CLK_SCLK);
    hal_clk_enable(CLK_MIPIDSI);

    hal_reset_deassert(RESET_MIPIDSI);

    aic_dsi_release_drvdata();
    return 0;
}

static int aic_dsi_clk_disable(void)
{
    hal_reset_assert(RESET_MIPIDSI);

    hal_clk_disable(CLK_MIPIDSI);
    hal_clk_disable(CLK_SCLK);
    return 0;
}

static int aic_dsi_enable(void)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();
    struct panel_dsi *dsi = comp->dsi;
    ulong lp_rate = MIPI_DSI_LP_RATE;

    reg_set_bit(comp->regs + DSI_CTL, DSI_CTL_EN);

    dsi_set_lane_assign(comp->regs, comp->ln_assign);
    dsi_set_lane_polrs(comp->regs, comp->ln_polrs);
    dsi_set_data_clk_polrs(comp->regs, comp->dc_inv);

    if (lp_rate < 10000000 || lp_rate > 20000000) {
        lp_rate = 10000000;
        pr_warn("Invalid lp rate, use default lp rate: %ld\n", lp_rate);
    }

    dsi_set_clk_div(comp->regs, comp->sclk_rate, lp_rate);
    dsi_pkg_init(comp->regs, dsi->mode);
    dsi_phy_init(comp->regs, comp->sclk_rate, dsi->lane_num, dsi->mode);
    dsi_hs_clk(comp->regs, 1);

    dsi_interrupt_init(comp->regs, DSI_IRQ_EN1_ALL, DSI_IRQ_EN2_ALL);

    aic_dsi_release_drvdata();
    return 0;
}

static int aic_dsi_disable(void)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();

    reg_clr_bit(comp->regs + DSI_CTL, DSI_CTL_EN);
    aic_dsi_release_drvdata();
    return 0;
}

static int aic_dsi_attach_panel(struct aic_panel *panel, struct panel_desc *desc)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();
    u32 div[DSI_MAX_LANE_NUM] = {24, 24, 18, 16};
    struct panel_dsi *dsi;
    u32 pixclk;

    if (desc) {
        pixclk = desc->timings->pixelclock;
        comp->timing = desc->timings;
        dsi = desc->dsi;
    } else {
        pixclk = panel->timings->pixelclock;
        comp->timing = panel->timings;
        dsi = panel->dsi;
    }
    comp->dsi = dsi;
    comp->panel = panel;
    comp->pixclk = pixclk;

    if (dsi->lane_num <= DSI_MAX_LANE_NUM)
    {
        comp->sclk_rate = pixclk * div[dsi->format] / dsi->lane_num;
    }
    else
    {
        comp->sclk_rate = 6; /* default RGB888 format 4 lane */
        pr_err("Invalid lane number %d\n", dsi->lane_num);
        return -EINVAL;
    }

    comp->pll_disp_rate = comp->sclk_rate;

    if (dsi->ln_assign)
        comp->ln_assign = dsi->ln_assign;
    else
        comp->ln_assign = LANE_ASSIGNMENTS;

    if (dsi->ln_polrs)
        comp->ln_polrs = dsi->ln_polrs;
    else
        comp->ln_polrs = LANE_POLARITIES;

    if (dsi->dc_inv)
        comp->dc_inv = dsi->dc_inv;
    else
        comp->dc_inv = CLK_INVERSE;

    aic_dsi_release_drvdata();
    return 0;
}

static int aic_dsi_set_vm(const struct display_timing *timing, int enable)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();
    struct panel_dsi *dsi = comp->dsi;

    if (enable) {
        dsi_dcs_lw(comp->regs, false);
        dsi_set_vm(comp->regs, dsi->mode, dsi->format,
            dsi->lane_num, comp->vc_num, timing);
    } else {
        dsi_set_vm(comp->regs, DSI_MOD_CMD_MODE, dsi->format,
            dsi->lane_num, comp->vc_num, timing);
        dsi_dcs_lw(comp->regs, true);
#if DCS_GET_DISPLAY_ID
        dsi_cmd_wr(comp->regs, MIPI_DSI_DCS_READ, 0,
                (u8[]){ MIPI_DCS_GET_DISPLAY_ID }, 1);
        aic_delay_ms(120);

        pr_info("mipi-dsi screen id: %x\n", readl(comp->regs + DSI_GEN_PD_CFG));
#endif
    }

    aic_dsi_release_drvdata();
    return 0;
}

static void aic_dsi_send_debug_cmd(struct aic_dsi_comp *comp)
{
    struct dsi_command *commands = &comp->dsi->command;
    unsigned int i = 0;

    aic_dsi_set_vm(comp->timing, false);
    while (i < commands->len) {
        u8 command = commands->buf[i++];
        u8 num_parameters = commands->buf[i++];
        const u8 *parameters = &commands->buf[i];

        if (command == 0x00 && num_parameters == 1)
            aic_delay_ms(parameters[0]);
        else
            dsi_cmd_wr(comp->regs, command, comp->vc_num, parameters, num_parameters);

        i += num_parameters;
    }
    aic_dsi_set_vm(comp->timing, true);
}

static int aic_dsi_send_cmd(u32 dt, u32 vc, const u8 *data, u32 len)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();
    struct panel_dsi *dsi = comp->dsi;

    if (dsi->command.command_on) {
        aic_dsi_send_debug_cmd(comp);
        return 0;
    }

    dsi_cmd_wr(comp->regs, dt, vc, data, len);
    aic_dsi_release_drvdata();
    return 0;
}

static int aic_dsi_read_cmd(u32 val)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();

    dsi_cmd_wr(comp->regs, MIPI_DSI_DCS_READ, 0, (u8[]){ val }, 1);
    aic_delay_ms(120);

    return readl(comp->regs + DSI_GEN_PD_CFG);
}

static int aic_dsi_get_output_bpp(void)
{
    struct aic_dsi_comp *comp = aic_dsi_request_drvdata();
    struct panel_dsi *dsi = comp->dsi;
    int bpp;

    switch (dsi->format) {
    case DSI_FMT_RGB888:
        bpp = 24;
        break;
    case DSI_FMT_RGB666L:
    case DSI_FMT_RGB666:
        bpp = 18;
        break;
    case DSI_FMT_RGB565:
        bpp = 16;
        break;
    default:
        bpp = 24;
        break;
    }

    aic_dsi_release_drvdata();
    return bpp;
}

struct di_funcs aic_dsi_func = {
    .clk_enable = aic_dsi_clk_enable,
    .clk_disable = aic_dsi_clk_disable,
    .enable = aic_dsi_enable,
    .disable = aic_dsi_disable,
    .attach_panel = aic_dsi_attach_panel,
    .set_videomode = aic_dsi_set_vm,
    .send_cmd = aic_dsi_send_cmd,
    .read_cmd = aic_dsi_read_cmd,
    .get_output_bpp = aic_dsi_get_output_bpp,
};

static int aic_dsi_probe(void)
{
    struct aic_dsi_comp *comp;

    comp = aicos_malloc(0, sizeof(*comp));
    if (!comp)
    {
        pr_err("allloc dsi comp failed\n");
        return -ENOMEM;
    }

    memset(comp, 0, sizeof(*comp));

    comp->regs     = (void *)MIPI_DSI_BASE;
    comp->vc_num   = VIRTUAL_CHANNEL;
    g_aic_dsi_comp = comp;

#ifdef AIC_DISP_PQ_TOOL
    AIC_PQ_SET_DSIP_PINMUX;
#endif

    aicos_request_irq(MIPI_DSI_IRQn, aic_dsi_handler, 0, "mipi-dsi", NULL);

    return 0;
}

static void aic_dsi_remove(void)
{

}

struct platform_driver artinchip_dsi_driver = {
    .name = "artinchip-dsi",
    .component_type = AIC_MIPI_COM,
    .probe = aic_dsi_probe,
    .remove = aic_dsi_remove,
    .di_funcs = &aic_dsi_func,
};

