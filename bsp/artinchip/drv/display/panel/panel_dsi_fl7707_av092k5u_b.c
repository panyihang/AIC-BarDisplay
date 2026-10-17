/*
 * Copyright (C) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FL7707N driver IC, AV092K5U-N10 panel, 462 x 1920, 4-lane MIPI-DSI
 *
 * Board variant "av092k5u-b-nor": DSI data lanes are wired in reverse
 * order compared to the default SoC pin map, panel control pins moved
 * to PD.15/PD.16/PD.17:
 *   D0_P=PD27, D1_P=PD25, CLK_P=PD23, D2_P=PD21, D3_P=PD19
 *   D0_N=PD26, D1_N=PD24, CLK_N=PD22, D2_N=PD20, D3_N=PD18
 *   DSI_RST=PD.17, LCD_PEN=PD.16, BL_PWM=PD.15
 * So lane assignment is reversed (0x0123) while P/N polarities stay
 * normal (N=even pin, P=odd pin).
 *
 * Init sequence from:
 * 36163809862.FL7707N_AV092K5U-N10_IPS_Code_Power_V4.3_20230413(2).txt
 */

#include "panel_com.h"
#include "panel_dsi.h"

static struct gpio_desc pen;    /* LCD_PEN, active low, keep low after power-on */
static struct gpio_desc reset;  /* DSI_RST */

static int panel_prepare(void)
{
    /*
     * LCD_PEN must be pulled low immediately after power-on.
     * It stays low while the panel is powered.
     */
    panel_get_gpio(&pen, AIC_PANEL_FL7707_AV092K5U_B_PEN_GPIO);
    panel_gpio_set_value(&pen, 0);
    aic_delay_ms(50);

    /* DSI_RST: reset pulse */
    panel_get_gpio(&reset, AIC_PANEL_FL7707_AV092K5U_B_RESET_GPIO);
    panel_gpio_set_value(&reset, 0);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(120);

    return 0;
}

static int panel_enable(struct aic_panel *panel)
{
    int ret;

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_dcs_send_seq(panel, 0xB9, 0xF1, 0x12, 0x87);
    panel_dsi_dcs_send_seq(panel, 0xB2, 0xE0, 0x05, 0x78);
    panel_dsi_dcs_send_seq(panel, 0xB3, 0x10, 0x10, 0x28, 0x28, 0x03, 0xFF,
                           0x00, 0x00, 0x00, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xB4, 0x80);
    panel_dsi_dcs_send_seq(panel, 0xB5, 0x08, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xB6, 0x68, 0x68);
    panel_dsi_dcs_send_seq(panel, 0xB8, 0x25, 0x22, 0xF0, 0x63);
    panel_dsi_dcs_send_seq(panel, 0xBA, 0x33, 0x81, 0x05, 0xF9, 0x0E, 0x0E,
                           0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x44, 0x25, 0x00, 0x91, 0x0A, 0x00, 0x00, 0x01,
                           0x4F, 0x01, 0x00, 0x00, 0x37);
    panel_dsi_dcs_send_seq(panel, 0xBC, 0x47);
    panel_dsi_dcs_send_seq(panel, 0xBF, 0x02, 0x10, 0x00, 0x80, 0x04);
    panel_dsi_dcs_send_seq(panel, 0xC0, 0x73, 0x73, 0x50, 0x50, 0x00, 0x00,
                           0x12, 0x73, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x25, 0x00, 0x32, 0x32, 0x77, 0xE4,
                           0x77, 0x77, 0xCC, 0xCC, 0xFF, 0xFF, 0x11, 0x11,
                           0x00, 0x00, 0x32);
    panel_dsi_dcs_send_seq(panel, 0xC7, 0x10, 0x00, 0x0A, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0xED, 0xC7, 0x00, 0xA5);
    panel_dsi_dcs_send_seq(panel, 0xC8, 0x10, 0x40, 0x1E, 0x03);
    panel_dsi_dcs_send_seq(panel, 0xCC, 0x07);
    panel_dsi_dcs_send_seq(panel, 0xE0, 0x00, 0x09, 0x0F, 0x2B, 0x3D, 0x3F,
                           0x44, 0x3D, 0x06, 0x0B, 0x0E, 0x12, 0x15, 0x12,
                           0x14, 0x13, 0x1A, 0x00, 0x09, 0x0F, 0x2B, 0x3D,
                           0x3F, 0x44, 0x3D, 0x06, 0x0B, 0x0E, 0x12, 0x15,
                           0x12, 0x14, 0x13, 0x1A);
    panel_dsi_dcs_send_seq(panel, 0xE1, 0x11, 0x11, 0x91, 0x00, 0x00, 0x00,
                           0x00);
    panel_dsi_dcs_send_seq(panel, 0xE3, 0x07, 0x07, 0x0B, 0x0B, 0x0B, 0x0B,
                           0x00, 0x00, 0x00, 0x00, 0xFF, 0x04, 0xC0, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xE9, 0xC8, 0x10, 0x0A, 0x10, 0x0E, 0x80,
                           0x45, 0x12, 0x31, 0x23, 0x4F, 0x86, 0x80, 0x45,
                           0x47, 0x08, 0x00, 0xC0, 0x01, 0x00, 0x00, 0x00,
                           0x00, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x4A, 0xBF,
                           0x88, 0x11, 0x33, 0x55, 0x77, 0x31, 0x88, 0x88,
                           0x88, 0x4A, 0xBF, 0x88, 0x00, 0x22, 0x44, 0x66,
                           0x20, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x01,
                           0x00, 0x80, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00);
    panel_dsi_dcs_send_seq(panel, 0xEA, 0x96, 0x18, 0x00, 0x00, 0x00, 0xB4,
                           0x01, 0x0B, 0x41, 0x01, 0x02, 0x00, 0x4A, 0xB8,
                           0xF8, 0x66, 0x44, 0x22, 0x00, 0x02, 0x88, 0x88,
                           0x88, 0x4A, 0xB8, 0xF8, 0x77, 0x55, 0x33, 0x11,
                           0x13, 0x88, 0x88, 0x88, 0x23, 0x00, 0x00, 0x00,
                           0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x07, 0x8B, 0x00, 0x00,
                           0x40, 0x80, 0x81, 0x40, 0x80, 0x81, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xEF, 0xFF, 0xFF, 0x01);

    ret = panel_dsi_dcs_exit_sleep_mode(panel);
    if (ret < 0) {
        pr_err("Failed to exit sleep mode: %d\n", ret);
        return ret;
    }
    aic_delay_ms(250);

    ret = panel_dsi_dcs_set_display_on(panel);
    if (ret < 0) {
        pr_err("Failed to set display on: %d\n", ret);
        return ret;
    }
    aic_delay_ms(50);

    panel_dsi_setup_realmode(panel);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static int panel_disable(struct aic_panel *panel)
{
    panel_default_disable(panel);
    panel_gpio_set_value(&reset, 0);
    return 0;
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

/*
 * htotal = 462 + 118 + 118 + 22 = 720
 * vtotal = 1920 + 16 + 20 + 4 = 1960
 * pixelclock = 720 * 1960 * 60 = 84.672MHz
 */
static struct display_timing fl7707_av092k5u_timing = {
    .pixelclock = FIRMWARE_PIXELCLOCK,
    .hactive = FIRMWARE_WIDTH,
    .hfront_porch = FIRMWARE_HFRONT_PORCH,
    .hback_porch = FIRMWARE_HBACK_PORCH,
    .hsync_len = FIRMWARE_HSYNC_LEN,
    .vactive = FIRMWARE_HEIGHT,
    .vfront_porch = FIRMWARE_VFRONT_PORCH,
    .vback_porch = FIRMWARE_VBACK_PORCH,
    .vsync_len = FIRMWARE_VSYNC_LEN,
};

static struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = FIRMWARE_LANE_NUM,
    .vc_num = VIRTUAL_CHANNEL,
    /*
     * Board wires the data lanes in reverse order:
     *   SoC D3 pair PD26/PD27 -> D0, D2 pair PD24/PD25 -> D1,
     *   D1 pair PD20/PD21 -> D2, D0 pair PD18/PD19 -> D3,
     * clock pair PD22/PD23 stays in place.
     * So lane assignment is reversed (0x0123) while polarities remain
     * normal (N = even pin, P = odd pin).
     */
    .ln_assign = FIRMWARE_LN_ASSIGN,
    .ln_polrs = FIRMWARE_LN_POLRS,
    .dc_inv = 0,
};

struct aic_panel dsi_fl7707_av092k5u_b = {
    .name = "panel-fl7707-av092k5u-b",
    .timings = &fl7707_av092k5u_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
