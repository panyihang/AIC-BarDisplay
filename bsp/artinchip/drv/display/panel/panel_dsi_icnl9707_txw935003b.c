/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TXW935003B (TXW, 9.35-inch bar display), candidate ICNL9707 panel driver.
 * The vendor data sheet is not available yet.  This diagnostic build uses
 * the public Milk-V/CVI ICN9707 480x1920 command table verbatim and changes
 * only the host timing active width to the selected diagnostic width.
 */

#include "panel_com.h"
#include "panel_dsi.h"

/* C2[5:4] + C2_LO encode NL (vertical lines / 2), while C2[2:0] is RSO:
 * 0=720 RGB, 1=600 RGB, 2=640 RGB.  For 1920 lines NL=0x3c0, hence 0x31c0.
 * RSO is not a 440-pixel width field; the host-side active width is separate.
 * The panel glass is 440 pixels wide, so the host must send all 440 pixels. */
#define TXW_TEST_WIDTH       480
#define TXW_TEST_C2_HI       0x31 /* RSO=1: 600 RGB, reference ICNL9707 profile */
#define TXW_TEST_C2_LO       0xC0
#define TXW_TEST_HFP         40
#define TXW_TEST_HBP         40
#ifndef TXW_HOST_VPG_TEST
#define TXW_HOST_VPG_TEST    0
#endif
#ifndef TXW_PANEL_BIST
#define TXW_PANEL_BIST       1
#endif
#ifndef TXW_PANEL_READ_ID
#define TXW_PANEL_READ_ID    1
#endif
#ifndef TXW_ASYNC_INIT
#define TXW_ASYNC_INIT       1
#endif
#define TXW_PANEL_BIST_SEL   0x0080 /* 0x80 color bar; 0x0400 check; 0x0800 column flick */

/* ICNL9707 reference tables use DSI Generic Long Write (data type 0x29) for
 * extended command pages.  DCS long writes (0x39) are not interchangeable
 * here: the command byte is interpreted differently by the panel. */
#define txw_icnl9707_gen(panel, seq...) do { \
    panel_dsi_generic_send_seq(panel, seq); \
    aic_delay_ms(1); \
} while (0)

static struct gpio_desc pen;
static struct gpio_desc reset;
static int panel_enable_sync(struct aic_panel *panel);

static void txw_read_dcs(struct aic_panel *panel, u8 cmd)
{
    u8 c = cmd;
    int ret = panel->callbacks.di_send_cmd(MIPI_DSI_DCS_READ, 0, &c, 1);
    pr_info("TXW935003B read 0x%02x ret=%d\n", cmd, ret);
}

static void txw_panel_init_thread(void *arg)
{
    struct aic_panel *panel = (struct aic_panel *)arg;

    aic_delay_ms(3000);
    pr_info("TXW935003B delayed panel init: start\n");
    if (panel_enable_sync(panel) < 0)
        pr_err("TXW935003B delayed panel init failed\n");
}

static int panel_prepare(void)
{
    pr_info("TXW935003B prepare start (PEN=%s, RESET=%s)\n",
            AIC_PANEL_ICNL9707_TXW935003B_PEN_GPIO,
            AIC_PANEL_ICNL9707_TXW935003B_RESET_GPIO);
    panel_get_gpio(&pen, AIC_PANEL_ICNL9707_TXW935003B_PEN_GPIO);
    /* TXW LCD_PEN is active low and enables panel power when asserted. */
    panel_gpio_set_value(&pen, 0);
    aic_delay_ms(50);

    panel_get_gpio(&reset, AIC_PANEL_ICNL9707_TXW935003B_RESET_GPIO);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset, 0);
    aic_delay_ms(50);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(200);
    pr_info("TXW935003B prepare done\n");
    return 0;
}

static int panel_enable_sync(struct aic_panel *panel)
{
    int ret;

    pr_info("TXW935003B enable start (width=%u, height=1920, 4-lane RGB888)\n",
            (unsigned int)TXW_TEST_WIDTH);

    panel_di_enable(panel, 0);
    panel_de_timing_disable(panel, 0);
    panel_dsi_send_perpare(panel);

    /* Exact Milk-V/CVI ICN9707 480x1920 sequence.  The public table uses
     * DSI generic long writes (0x29) for all extended commands. */
    txw_icnl9707_gen(panel, 0xF0, 0x5A, 0x59);
    txw_icnl9707_gen(panel, 0xF1, 0xA5, 0xA6);
    txw_icnl9707_gen(panel, 0xB4, 0x1D, 0x1C, 0x0B, 0x10, 0x11, 0x12,
                     0x13, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x1E, 0x1F,
                     0x04, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03);
    txw_icnl9707_gen(panel, 0xB0, 0x60, 0x00, 0x00, 0x08, 0x66, 0x66,
                               0x33, 0x33, 0x66, 0x00, 0x00, 0xAF, 0x00, 0x00, 0x0F);
    txw_icnl9707_gen(panel, 0xB1, 0x53, 0xA0, 0x00, 0x85, 0x0E, 0x00,
                               0x00, 0x62, 0x00, 0x00);
    txw_icnl9707_gen(panel, 0xB2, 0x37, 0x09, 0x08, 0x8B, 0x08, 0x00,
                               0x22, 0x00, 0x44, 0xD9);
    txw_icnl9707_gen(panel, 0xB6, 0x49, 0x49);
    txw_icnl9707_gen(panel, 0xB7, 0x01, 0x01, 0x09, 0x0D, 0x11, 0x19,
                               0x1D, 0x15, 0x00, 0x25, 0x21, 0x00, 0x00, 0x00,
                               0x00, 0x02, 0xF7, 0x38);
    txw_icnl9707_gen(panel, 0xB8, 0x34, 0x53, 0x02, 0xCC);
    txw_icnl9707_gen(panel, 0xBA, 0x27, 0x33);
    txw_icnl9707_gen(panel, 0xBD, 0x43, 0x0E, 0x0E, 0x4B, 0x4B, 0x14, 0x14);
    txw_icnl9707_gen(panel, 0xC1, 0x00, 0x0C, 0x20, 0x04, 0x00, 0x32, 0x32, 0x04);
    txw_icnl9707_gen(panel, 0xC2, TXW_TEST_C2_HI, TXW_TEST_C2_LO);
    txw_icnl9707_gen(panel, 0xC3, 0x22, 0x31);
    txw_icnl9707_gen(panel, 0xC6, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00);
    txw_icnl9707_gen(panel, 0xC8, 0x7C, 0x66, 0x56, 0x49, 0x46, 0x37, 0x3A, 0x23,
                               0x3B, 0x38, 0x38, 0x55, 0x43, 0x4C, 0x40, 0x3C, 0x2F, 0x1C, 0x06,
                               0x7C, 0x65, 0x56, 0x4A, 0x46, 0x37, 0x3A, 0x23, 0x3A, 0x38, 0x38,
                               0x55, 0x43, 0x4C, 0x40, 0x3C, 0x2F, 0x1C, 0x06);
    txw_icnl9707_gen(panel, 0xD0, 0x07, 0xFF, 0xFF);
    txw_icnl9707_gen(panel, 0xD2, 0x63, 0x0B, 0x08, 0x88);
    txw_icnl9707_gen(panel, 0xD4, 0x00, 0x00, 0x00, 0x32, 0x04, 0x54);

    /* Leave the extended-command window after all panel setup/BIST writes. */
    txw_icnl9707_gen(panel, 0xF1, 0x5A, 0x59);
    txw_icnl9707_gen(panel, 0xF0, 0xA5, 0xA6);

    ret = panel_dsi_dcs_exit_sleep_mode(panel);
    if (ret < 0) {
        pr_err("TXW935003B sleep-out failed: %d\n", ret);
        return ret;
    }
    pr_info("TXW935003B sleep-out sent\n");
    /* Milk-V/CVI reference table: 200 ms after sleep-out. */
    aic_delay_ms(200);
    ret = panel_dsi_dcs_set_display_on(panel);
    if (ret < 0) {
        pr_err("TXW935003B display-on failed: %d\n", ret);
        return ret;
    }
    pr_info("TXW935003B display-on sent\n");
    aic_delay_ms(50);

#if TXW_PANEL_READ_ID
    /* Read after sleep-out/display-on: DDB and DCS status are only defined
     * once the panel's command engine is awake. */
    txw_read_dcs(panel, 0xA1);
    txw_read_dcs(panel, 0xDA);
    txw_read_dcs(panel, 0xDB);
    txw_read_dcs(panel, 0xDC);
    txw_read_dcs(panel, 0x09);
    txw_read_dcs(panel, 0x0A);
    txw_read_dcs(panel, 0x0B);
    txw_read_dcs(panel, 0x0C);
    txw_read_dcs(panel, 0x0F);
#endif

#if TXW_PANEL_BIST
    /* Program BIST last, after display-on and all identification reads.  The
     * previous sequence selected BIST and then sent F0/F1 plus 11/29, which
     * allowed the panel command state machine to leave/override BIST. */
    ret = mipi_dsi_dcs_send(panel, (u8[]){ 0xC0, 0x11 }, 2);
    pr_info("TXW935003B: panel BIST arm C0=11 dt=DCS_SHORT_PARAM ret=%d\n", ret);
    if (ret < 0)
        return ret;
    ret = mipi_dsi_dcs_send(panel, (u8[]){ 0xC0, 0x10,
                           (TXW_PANEL_BIST_SEL >> 8) & 0xff,
                           TXW_PANEL_BIST_SEL & 0xff }, 4);
    pr_info("TXW935003B: panel BIST select C0=10 sel=0x%04x dt=DCS_LONG ret=%d\n",
            TXW_PANEL_BIST_SEL, ret);
    if (ret < 0)
        return ret;
#endif

    panel_dsi_setup_realmode(panel);
#if TXW_HOST_VPG_TEST
    /* DSI host VPG is independent of the DE FIFO. */
    pr_info("TXW935003B: DSI host VPG active; DE timing bypassed\n");
#else
    panel_de_timing_enable(panel, 0);
#endif
    /* The asynchronous enable path returns before panel_com's normal
     * backlight step.  Turn the panel backlight on explicitly after DSI and
     * DE are ready. */
    panel_backlight_enable(panel, 0);
    pr_info("TXW935003B backlight enabled\n");
    pr_info("TXW935003B enable done\n");
    return 0;
}

static int panel_enable(struct aic_panel *panel)
{
#if !TXW_ASYNC_INIT
    return panel_enable_sync(panel);
#else
    static aicos_thread_t worker;

    if (worker)
        return 0;
    worker = aicos_thread_create("txw_dsi", 8192, 18,
                                 txw_panel_init_thread, panel);
    if (!worker)
        return -ENOMEM;
    pr_info("TXW935003B: panel init delayed 3000 ms for USB CDC log capture\n");
    return 0;
#endif
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

/* Diagnostic profile copied from the published ICNL9707 480x1920 reference.
 * The TXW935003B mechanical dimensions match that family more closely than
 * the contradictory 440x1920 listing, so this is the decisive glass-width
 * experiment. */
static struct display_timing txw935003b_timing = {
    /* 480 + 53 + 53 + 4 by 1920 + 32 + 12 + 4 = 69.66 MHz @ 60 Hz. */
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
    /* ICNL9707 reference uses burst video mode.  In burst mode the host
     * emits the active packet as one line and does not subtract HSA/HBP from
     * the packet boundary. */
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = FIRMWARE_LANE_NUM,
    /* ICNL9707 is specified for <=550 Mbps/lane in 4-lane mode. */
    .vc_num = VIRTUAL_CHANNEL,
    .ln_assign = FIRMWARE_LN_ASSIGN,
    .ln_polrs = FIRMWARE_LN_POLRS,
    .dc_inv = 0,
#if TXW_HOST_VPG_TEST
    .vpg_enable = 1,
    .vpg_mode = 0,
    .vpg_orientation = 0,
#endif
};

struct aic_panel dsi_icnl9707_txw935003b = {
    .name = "panel-icnl9707-txw935003b",
    .timings = &txw935003b_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
