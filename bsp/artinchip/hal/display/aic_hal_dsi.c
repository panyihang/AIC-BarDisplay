/*
 * Copyright (C) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_hal_dsi.h>
#include <aic_iopoll.h>

#define DSI_TIMEOUT_US  1000000

void dsi_set_lane_assign(void *base, u32 ln_assign)
{
    reg_write(base + DSI_LANE_CFG, ln_assign);
}

void dsi_set_lane_polrs(void *base, u32 ln_polrs)
{
    reg_set_bits(base + DSI_ANA_CFG1,
            DSI_DATA_LANE_POL_MASK, DSI_DATA_LANE_POL(ln_polrs));
}

void dsi_set_data_clk_polrs(void *base, u32 dc_inv)
{
    if (dc_inv)
        reg_set_bit(base + DSI_ANA_CFG1, DSI_DATA_CLK_POL);
    else
        reg_clr_bit(base + DSI_ANA_CFG1, DSI_DATA_CLK_POL);
}

void dsi_interrupt_init(void *base, u32 irq1_flag, u32 irq2_flag)
{
    if (irq1_flag)
        reg_write(base + DSI_IRQ_EN1, irq1_flag);

    if (irq2_flag)
        reg_write(base + DSI_IRQ_EN2, irq2_flag);
}

void dsi_dcs_lw(void *base, u32 enable)
{
    if (enable)
        reg_set_bit(base + DSI_CMD_MODE_CFG, DSI_CMD_MODE_CFG_DCS_LW);
    else
        reg_clr_bit(base + DSI_CMD_MODE_CFG, DSI_CMD_MODE_CFG_DCS_LW);
}

void dsi_set_clk_div(void *base, ulong mclk, ulong lp_rate)
{
    u32 div;

    div = mclk / 8 / lp_rate;
    reg_set_bits(base + DSI_CLK_CFG,
        DSI_CLK_CFG_TO_DIV_MASK | DSI_CLK_CFG_LP_DIV_MASK,
        DSI_CLK_CFG_TO_DIV(div) | DSI_CLK_CFG_LP_DIV(div));
}

void dsi_pkg_init(void *base, enum dsi_mode mode)
{
    reg_set_bits(base + DSI_CTL,
        DSI_CTL_PKG_CFG_CRC_RX_EN | DSI_CTL_PKG_CFG_ECC_RX_EN,
        DSI_CTL_PKG_CFG_CRC_RX_EN | DSI_CTL_PKG_CFG_ECC_RX_EN);

    if (mode & DSI_MOD_NO_EOT_PACKET)
        reg_clr_bit(base + DSI_CTL, DSI_CTL_PKG_CFG_EOTP_TX_EN);
    else
        reg_set_bit(base + DSI_CTL, DSI_CTL_PKG_CFG_EOTP_TX_EN);

    reg_set_bits(base + DSI_DPI_LPTX_TIME,
        DSI_DPI_LPTX_TIME_OUTVACT_MASK | DSI_DPI_LPTX_TIME_INVACT_MASK,
        DSI_DPI_LPTX_TIME_OUTVACT(64) | DSI_DPI_LPTX_TIME_INVACT(64));
    reg_set_bits(base + DSI_TO_CNT_CFG,
        DSI_TO_CNT_CFG_HSTX_MASK | DSI_TO_CNT_CFG_LPRX_MASK,
        DSI_TO_CNT_CFG_HSTX(0) | DSI_TO_CNT_CFG_LPRX(0));
    reg_write(base + DSI_CMD_MODE_CFG,
        DSI_CMD_MODE_CFG_MAX_RD_PKG_SIZE |
        DSI_CMD_MODE_CFG_DCS_LW | DSI_CMD_MODE_CFG_DCS_SR_0P |
        DSI_CMD_MODE_CFG_DCS_SW_1P | DSI_CMD_MODE_CFG_DCS_SW_0P |DSI_CMD_MODE_CFG_GEN_LW | DSI_CMD_MODE_CFG_GEN_SR_2P |
        DSI_CMD_MODE_CFG_GEN_SR_1P | DSI_CMD_MODE_CFG_GEN_SR_0P |
        DSI_CMD_MODE_CFG_GEN_SW_2P | DSI_CMD_MODE_CFG_GEN_SW_1P |
        DSI_CMD_MODE_CFG_GEN_SW_0P);
}

static void dsi_dphy_cfg(void *base,
        u32 code, u32 *p_wdata, u32 *p_rdata)
{
    void *TST1 = base + DSI_PHY_TEST1;
    void *TST2 = base + DSI_PHY_TEST2;

    reg_set_bit(TST2, DSI_PHY_TEST2_EN);
    aic_udelay(1);
    reg_set_bit(TST1, DSI_PHY_TEST1_CLK);
    aic_udelay(1);
    reg_set_bits(TST2, DSI_PHY_TEST2_DIN_MASK, DSI_PHY_TEST2_DIN(code));
    aic_udelay(1);
    reg_clr_bit(TST1, DSI_PHY_TEST1_CLK);
    aic_udelay(1);
    reg_clr_bit(TST2, DSI_PHY_TEST2_EN);
    aic_udelay(1);

    if (p_wdata && p_rdata) {
        u32 *pi = p_wdata;
        u32 *po = p_rdata;

        reg_set_bits(TST2, DSI_PHY_TEST2_DIN_MASK,
                DSI_PHY_TEST2_DIN(*pi++));
        aic_udelay(1);
        reg_set_bit(TST1, DSI_PHY_TEST1_CLK);
        aic_udelay(1);
        *po++ = reg_rd_bits(TST2,
            DSI_PHY_TEST2_DOUT_MASK, DSI_PHY_CFG_LP11_TIME_SHIFT);
        aic_udelay(1);
        reg_clr_bit(TST1, DSI_PHY_TEST1_CLK);
        aic_udelay(1);
    }
}

static void dsi_dphy_cfg_hsfreq(void *base, ulong mclk)
{
    u32 freq_rdata = mclk / 1000000;
    u32 freq_wdata;

    static const struct {
        u32 div;
        u32 value;
    } hs_clk_div[] = {
        { 89,   0x00 }, { 99,   0x10 }, { 109,  0x20 }, { 129,  0x01 },
        { 139,  0x11 }, { 149,  0x21 }, { 169,  0x02 }, { 179,  0x12 },
        { 199,  0x22 }, { 219,  0x03 }, { 239,  0x13 }, { 249,  0x23 },
        { 269,  0x04 }, { 299,  0x14 }, { 329,  0x05 }, { 359,  0x15 },
        { 399,  0x25 }, { 449,  0x06 }, { 499,  0x16 }, { 549,  0x07 },
        { 599,  0x17 }, { 649,  0x08 }, { 699,  0x18 }, { 749,  0x09 },
        { 799,  0x19 }, { 849,  0x29 }, { 899,  0x39 }, { 949,  0x0a },
        { 999,  0x1a }, { 1049, 0x2a }, { 1099, 0x3a }, { 1149, 0x0b },
        { 1199, 0x1b }, { 1249, 0x2b }, { 1299, 0x3b }, { 1349, 0x0c },
        { 1399, 0x1c }, { 1449, 0x2c }, { 1499, 0x3c },
    };

    u32 i, value = 0;

    for (i = 0; i < ARRAY_SIZE(hs_clk_div); ++i) {
        if (hs_clk_div[i].div >= freq_rdata) {
            value = hs_clk_div[i].value;
#ifdef AIC_DSI_LEGACY_PACKET_CONFIG
            value = value << 1;
#endif
            break;
        }
    }

    freq_wdata = value;

    dsi_dphy_cfg(base, 0x44, &freq_wdata, &freq_rdata);
}

void dsi_phy_init(void *base, ulong mclk, u32 lane, enum dsi_mode mode)
{
    void *ANA2 = base + DSI_ANA_CFG2;
    void *CFG = base + DSI_PHY_CFG;
    void *TST1 = base + DSI_PHY_TEST1;
    int ret;
    u32 val;

    reg_write(base + DSI_PHY_RD_TIME, 50);

    reg_set_bits(base + DSI_PHY_CLK_TIME,
        DSI_PHY_CLK_TIME_HS2LP_MASK | DSI_PHY_CLK_TIME_LP2HS_MASK,
        DSI_PHY_CLK_TIME_HS2LP(20) | DSI_PHY_CLK_TIME_LP2HS(20));
    reg_set_bits(base + DSI_PHY_DATA_TIME,
        DSI_PHY_DATA_TIME_HS2LP_MASK | DSI_PHY_DATA_TIME_LP2HS_MASK,
        DSI_PHY_DATA_TIME_HS2LP(20) | DSI_PHY_DATA_TIME_LP2HS(20));

    reg_set_bit(ANA2, DSI_ANA_CFG2_EN_BIAS);
    aic_udelay(5);
    reg_set_bits(ANA2, DSI_ANA_CFG2_EN_VP_MASK, DSI_ANA_CFG2_EN_VP(3));
    reg_set_bits(ANA2, DSI_ANA_CFG2_EN_LDO_MASK,
            DSI_ANA_CFG2_EN_LDO(0x1F));
    aic_udelay(10);
    reg_set_bit(ANA2, DSI_ANA_CFG2_EN_RESCAL);
    reg_set_bit(ANA2, DSI_ANA_CFG2_ON_RESCAL);
    ret = readl_poll_timeout(ANA2, val, val & DSI_ANA_CFG2_RCAL_FLAG,
            DSI_TIMEOUT_US);
    if (ret) {
        pr_err("Timeout during wait rcal flag\n");
        return;
    }

    reg_set_bit(ANA2, DSI_ANA_CFG2_EN_CLK);
    reg_set_bit(TST1, DSI_PHY_TEST1_CLR);
    aic_udelay(5);
    reg_clr_bit(CFG, DSI_PHY_CFG_RST_RSTN);
    aic_udelay(5);
    reg_clr_bit(CFG, DSI_PHY_CFG_RST_SHUTDOWNZ);
    aic_udelay(5);
    reg_clr_bit(TST1, DSI_PHY_TEST1_CLR);
    aic_udelay(5);

    dsi_dphy_cfg_hsfreq(base, mclk);
    dsi_dphy_cfg(base, 0, NULL, NULL);
    reg_set_bit(CFG, DSI_PHY_CFG_RST_SHUTDOWNZ);
    aic_udelay(5);
    reg_set_bit(CFG, DSI_PHY_CFG_RST_RSTN);
    aic_udelay(5);
    reg_set_bits(CFG, DSI_PHY_CFG_LP11_TIME_MASK,
        DSI_PHY_CFG_LP11_TIME(10));
    reg_set_bits(CFG, DSI_PHY_CFG_DATA_LANE_MASK,
        DSI_PHY_CFG_DATA_LANE(lane - 1));
    aic_udelay(5);

    if (mode & DSI_CLOCK_NON_CONTINUOUS) {
        reg_clr_bit(CFG, DSI_PHY_CFG_HSCLK_REQ);
        aic_udelay(5);
        reg_set_bit(CFG, DSI_PHY_CFG_AUTO_CLK_EN);
        aic_udelay(5);
    }

    reg_set_bit(CFG, DSI_PHY_CFG_RST_CLK_EN);
}

void dsi_hs_clk(void *base, u32 enable)
{
    int ret;
    u32 val;

    if (enable) {
            ret = readl_poll_timeout(base + DSI_PHY_STA,
                    val, val & DSI_PHY_STA_STOP_STATE_C,
                    DSI_TIMEOUT_US);
            if (ret) {
                    pr_err("Timeout during wait phy stop state c\n");
                    return;
            }

            reg_set_bit(base + DSI_PHY_CFG, DSI_PHY_CFG_HSCLK_REQ);
            return;
    }

    reg_clr_bit(base + DSI_PHY_CFG, DSI_PHY_CFG_HSCLK_REQ);
    ret =  readl_poll_timeout(base + DSI_PHY_STA,
            val, val & DSI_PHY_STA_STOP_STATE_C,
            DSI_TIMEOUT_US);
    if (ret)
            pr_err("Timeout during wait phy stop state c\n");
}

void dsi_set_vm(void *base, enum dsi_mode mode, enum dsi_format format,
        u32 lane, u32 vc, const struct display_timing *timing)
{
    void *IFCFG = base + DSI_DPI_IF_CFG;
    void *INFMT = base + DSI_DPI_IN_FMT;
    void *INPOL = base + DSI_DPI_IN_POL;
    void *CMDCFG = base + DSI_CMD_MODE_CFG;
    void *VIDCFG = base + DSI_VID_MODE_CFG;
    u32 if_cfg_fmt[] = {0, 0, 1, 2};
    u32 in_fmt_dt[] = {5, 3, 3, 1};
    u32 dsi_bits_per_pixel[] = {24, 24, 18, 16};
    u32 ht = timing->hback_porch + timing->hsync_len + timing->hactive +
        timing->hfront_porch;

    if (unlikely(format >= DSI_FMT_MAX))
        BUG();

    reg_clr_bit(IFCFG, DSI_DPI_IF_CFG_SHUTD);
    reg_clr_bit(IFCFG, DSI_DPI_IF_CFG_COLORM);
    reg_set_bits(IFCFG, DSI_DPI_IF_CFG_FMT_MASK,
        DSI_DPI_IF_CFG_FMT(if_cfg_fmt[format]));
    reg_set_bits(INFMT, DSI_DPI_IN_FMT_DT_MASK,
        DSI_DPI_IN_FMT_DT(in_fmt_dt[format]));
    if (format == DSI_FMT_RGB666L)
        reg_set_bit(INFMT, DSI_DPI_IN_FMT_LOOSELY18);
    else
        reg_clr_bit(INFMT, DSI_DPI_IN_FMT_LOOSELY18);

    reg_set_bits(base + DSI_DPI_VC, DSI_DPI_VC_NUM_MASK,
            DSI_DPI_VC_NUM(vc));
    reg_clr_bit(INPOL, DSI_DPI_IN_POL_DE);
    reg_set_bit(INPOL, DSI_DPI_IN_POL_VSYNC);
    reg_set_bit(INPOL, DSI_DPI_IN_POL_HSYNC);
    reg_clr_bit(INPOL, DSI_DPI_IN_POL_SHUTDOWN);
    reg_clr_bit(INPOL, DSI_DPI_IN_POL_COLORM);

    if (mode & DSI_MOD_CMD_MODE) {
        reg_set_bit(base + DSI_CTL, DSI_CTL_DSI_MODE);
        reg_write(base + DSI_EDPI_CMD_SIZE, timing->hactive);
        reg_clr_bit(CMDCFG, DSI_CMD_MODE_CFG_ACK_REQ_EN);
        reg_clr_bit(CMDCFG, DSI_CMD_MODE_CFG_TE_EN);
        reg_clr_bit(CMDCFG, DSI_CMD_MODE_CFG_DCS_LW);
        return;
    }

    reg_clr_bit(base + DSI_CTL, DSI_CTL_DSI_MODE);
    reg_set_bits(VIDCFG, DSI_VID_MODE_CFG_TYPE_MASK,
            DSI_VID_MODE_CFG_TYPE(mode));
    reg_set_bit(VIDCFG, DSI_VID_MODE_CFG_LP_EN_VSA);
    reg_set_bit(VIDCFG, DSI_VID_MODE_CFG_LP_EN_VBP);
    reg_set_bit(VIDCFG, DSI_VID_MODE_CFG_LP_EN_VFP);
    reg_set_bit(VIDCFG, DSI_VID_MODE_CFG_LP_EN_VACT);

    if (mode & DSI_MOD_VID_HBP)
        reg_clr_bit(VIDCFG, DSI_VID_MODE_CFG_LP_EN_HBP);
    else
        reg_set_bit(VIDCFG, DSI_VID_MODE_CFG_LP_EN_HBP);

    if (mode & DSI_MOD_VID_BURST)
        reg_set_bit(VIDCFG, DSI_VID_MODE_CFG_LP_EN_HFP);
    else
        reg_clr_bit(VIDCFG, DSI_VID_MODE_CFG_LP_EN_HFP);

    reg_clr_bit(VIDCFG, DSI_VID_MODE_CFG_FRAME_BTA_ACK_EN);
    reg_set_bit(VIDCFG, DSI_VID_MODE_CFG_CMD_LPTX_FORCE);

    reg_write(base + DSI_VID_PKG_SIZE, timing->hactive);
    reg_write(base + DSI_VID_CHK_NUM, 1);
    reg_write(base + DSI_VID_NULL_SIZE, 0);

    reg_set_bits(base + DSI_VID_HINACT_TIME,
        DSI_VID_HSA_TIME_MASK,
        DSI_VID_HSA_TIME(
        timing->hsync_len * dsi_bits_per_pixel[format]/(8 * lane)));

    reg_set_bits(base + DSI_VID_HINACT_TIME,
        DSI_VID_HBP_TIME_MASK,
        DSI_VID_HBP_TIME(
            timing->hback_porch *
            dsi_bits_per_pixel[format]/(8 * lane)));

    reg_write(base + DSI_VID_HT_TIME,
        ht * dsi_bits_per_pixel[format]/(8 * lane));

    reg_set_bits(base + DSI_VID_VACT_LINE,
        DSI_VID_VSA_LINE_MASK,
        DSI_VID_VSA_LINE(timing->vsync_len));
    reg_set_bits(base + DSI_VID_VBLANK_LINE,
        DSI_VID_VBP_TIME_MASK,
        DSI_VID_VBP_TIME(timing->vback_porch));
    reg_set_bits(base + DSI_VID_VBLANK_LINE,
        DSI_VID_VFP_LINE_MASK,
        DSI_VID_VFP_LINE(timing->vfront_porch));
    reg_set_bits(base + DSI_VID_VACT_LINE,
        DSI_VID_VACT_TIME_MASK,
        DSI_VID_VACT_TIME(timing->vactive));
}

static void dsi_cmd_set_dt(void *base, u32 dt)
{
    switch (dt) {
    case DSI_DT_GEN_WR_P0:
        reg_set_bit(base, DSI_CMD_MODE_CFG_GEN_SW_0P);
        break;
    case DSI_DT_GEN_WR_P1:
        reg_set_bit(base, DSI_CMD_MODE_CFG_GEN_SW_1P);
        break;
    case DSI_DT_GEN_WR_P2:
        reg_set_bit(base, DSI_CMD_MODE_CFG_GEN_SW_2P);
        break;
    case DSI_DT_GEN_RD_P0:
        reg_set_bit(base, DSI_CMD_MODE_CFG_GEN_SR_0P);
        break;
    case DSI_DT_GEN_RD_P1:
        reg_set_bit(base, DSI_CMD_MODE_CFG_GEN_SR_1P);
        break;
    case DSI_DT_GEN_RD_P2:
        reg_set_bit(base, DSI_CMD_MODE_CFG_GEN_SR_2P);
        break;
    case DSI_DT_GEN_LONG_WR:
        reg_set_bit(base, DSI_CMD_MODE_CFG_GEN_LW);
        break;
    case DSI_DT_DCS_WR_P0:
        reg_set_bit(base, DSI_CMD_MODE_CFG_DCS_SW_0P);
        break;
    case DSI_DT_DCS_WR_P1:
        reg_set_bit(base, DSI_CMD_MODE_CFG_DCS_SW_1P);
        break;
    case DSI_DT_DCS_RD_P0:
        reg_set_bit(base, DSI_CMD_MODE_CFG_DCS_SR_0P);
        break;
    case DSI_DT_DCS_LONG_WR:
        reg_set_bit(base, DSI_CMD_MODE_CFG_DCS_LW);
        break;
    case DSI_DT_MAX_RET_SIZE:
        reg_set_bit(base, DSI_CMD_MODE_CFG_MAX_RD_PKG_SIZE);
        break;
    }
}

void dsi_cmd_wr(void *base, u32 dt, u32 vc, const u8 *data, u32 len)
{
    const u8 *p = data;
    u8 d0, d1, i;
    u32 val;
    int ret;
    void *CMDCFG = base + DSI_CMD_MODE_CFG;

    switch (dt) {
    case DSI_DT_GEN_WR_P0:
    case DSI_DT_GEN_RD_P0:
        d0 = 0; d1 = 0; i = 0;
        break;
    case DSI_DT_GEN_WR_P1:
    case DSI_DT_GEN_RD_P1:
    case DSI_DT_DCS_WR_P0:
    case DSI_DT_DCS_RD_P0:
        d0 = *p++; d1 = 0; i = 1;
        break;
    case DSI_DT_GEN_WR_P2:
    case DSI_DT_GEN_RD_P2:
    case DSI_DT_DCS_WR_P1:
    case DSI_DT_MAX_RET_SIZE:
        d0 = *p++; d1 = *p++; i = 2;
        break;
    case DSI_DT_GEN_LONG_WR:
    case DSI_DT_DCS_LONG_WR:
        d0 = len >> 0; d1 = (len >> 8) & 0xff; i = 0;
        break;
    default:
        d0 = 0; d1 = 0; i = 0;
        break;
    }

    dsi_cmd_set_dt(CMDCFG, dt);

    if (dt == DSI_DT_GEN_RD_P0 || dt == DSI_DT_GEN_RD_P1 ||
        dt == DSI_DT_GEN_RD_P2 || dt == DSI_DT_DCS_RD_P0) {
        reg_clr_bit(CMDCFG, DSI_CMD_MODE_CFG_ACK_REQ_EN);
        reg_set_bit(base + DSI_CTL, DSI_CTL_PKG_CFG_BTA_EN);
    } else {
        reg_clr_bit(CMDCFG, DSI_CMD_MODE_CFG_ACK_REQ_EN);
        reg_clr_bit(base + DSI_CTL, DSI_CTL_PKG_CFG_BTA_EN);
    }

    for (; i < len; i += 4)
        reg_write(base + DSI_GEN_PD_CFG,
            (*(p + i + 3) << 24) | (*(p + i + 2) << 16) |
            (*(p + i + 1) <<  8) | (*(p + i + 0) << 0));

    ret = readl_poll_timeout(base + DSI_PHY_STA,
            val, val & DSI_PHY_STA_STOP_STATE_0,
            DSI_TIMEOUT_US);
    if (ret) {
        pr_err("Timeout during wait phy stop state 0\n");
        return;
    }

    reg_write(base + DSI_GEN_PH_CFG,
        (d1 << 16) | (d0 << 8) | (vc << 6) | dt);

    ret = readl_poll_timeout(base + DSI_CMD_PKG_STA,
            val, val & DSI_CMD_PKG_STA_PLD_W_EMPTY,
            DSI_TIMEOUT_US);
    if (ret) {
        pr_err("failed to write pld write fifo\n");
        return;
    }

    aic_udelay(10);
}

