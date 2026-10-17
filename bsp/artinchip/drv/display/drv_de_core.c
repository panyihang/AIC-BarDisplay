/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drv_fb.h"
#include "drv_de.h"
#include <aic_iopoll.h>

#ifndef AIC_DE_DRV_V10
#include "disp_ccm.h"
#include "disp_gamma.h"
#endif

#define DITHER_RGB565   0x1
#define DITHER_RGB666   0x2

#define DE_TIMEOUT_MS     100
#define DE_PREFETCH_LINE  2

static struct aic_de_comp *g_aic_de_comp;

static int aic_de_set_gamma_config(struct aicfb_gamma_config *gamma);
static int aic_de_set_ccm_config(struct aicfb_ccm_config *ccm);

static struct aic_de_comp *aic_de_request_drvdata(void)
{
    return g_aic_de_comp;
}

static void aic_de_release_drvdata(void)
{

}

static irqreturn_t aic_de_handler(int irq, void *ctx)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    u32 status;

    status = de_timing_interrupt_status(comp->regs);
    de_timing_interrupt_clean_status(comp->regs, status);

#ifdef AIC_DE_DRV_V11_1
    if (status & TIMING_INIT_CFG_DONE_STATUS)
        aicos_wqueue_wakeup(comp->vsync_queue);
#endif

    if (status & TIMING_INIT_V_BLANK_FLAG) {
#ifndef AIC_DE_DRV_V11_1
        aicos_wqueue_wakeup(comp->vsync_queue);
#endif
        if (comp->cb)
            comp->cb(comp->cb_data);
    }

    if (comp->scaler_active & SCALER0_CTRL_ACTIVE) {
        comp->scaler_active = comp->scaler_active & 0xF;

        de_scaler0_active_handle(comp->regs, comp->scaler_active);
    }


    if (status & TIMING_INIT_UNDERFLOW_FLAG)
        pr_err("ERROR: DE UNDERFLOW\n");

    return IRQ_HANDLED;
}

void de_register_vsync_cb(de_vsync_cb cb, void *data)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    size_t flag;

    flag = aicos_enter_critical_section();
    /* INTERRUPT Context */
    comp->cb = cb;
    comp->cb_data = data;
    aicos_leave_critical_section(flag);
}

void de_unregister_vsync_cb(void)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    size_t flag;

    flag = aicos_enter_critical_section();
    comp->cb = NULL;
    comp->cb_data = NULL;
    aicos_leave_critical_section(flag);
}

static inline bool is_valid_layer_id(struct aic_de_comp *comp, u32 layer_id)
{
#if defined(AIC_DE_DRV_V12) || defined(AIC_DE_V12)
    if (layer_id != AICFB_LAYER_TYPE_UI)
        return false;
    else
        return true;
#else
    u32 total_num = comp->config->layer_num->vi_num
            + comp->config->layer_num->ui_num;

    if (layer_id < total_num)
        return true;
    else
        return false;
#endif
}

static inline bool need_update_disp_prop(struct aic_de_comp *comp,
                     struct aicfb_disp_prop *disp_prop)
{
    if (disp_prop->bright != comp->disp_prop.bright ||
        disp_prop->contrast != comp->disp_prop.contrast ||
        disp_prop->saturation != comp->disp_prop.saturation ||
        disp_prop->hue != comp->disp_prop.hue)
        return true;

    return false;
}

static inline bool is_valid_layer_and_rect_id(struct aic_de_comp *comp,
                          u32 layer_id, u32 rect_id)
{
    u32 flags;

    if (!is_valid_layer_id(comp, layer_id))
        return false;

    flags = comp->config->cap[layer_id].cap_flags;

    if (flags & AICFB_CAP_4_RECT_WIN_FLAG) {
        if (layer_id != 0 && rect_id >= MAX_RECT_NUM)
            return false;
    }
    return true;
}

static inline bool is_support_color_key(struct aic_de_comp *comp,
                    u32 layer_id)
{
    u32 flags;

    if (!is_valid_layer_id(comp, layer_id))
        return false;

    flags = comp->config->cap[layer_id].cap_flags;

    if (flags & AICFB_CAP_CK_FLAG)
        return true;
    else
        return false;
}

static inline bool is_support_alpha_blending(struct aic_de_comp *comp,
                         u32 layer_id)
{
    u32 flags;

    if (!is_valid_layer_id(comp, layer_id))
        return false;

    flags = comp->config->cap[layer_id].cap_flags;

    if (flags & AICFB_CAP_ALPHA_FLAG)
        return true;
    else
        return false;
}

static inline void de_check_ui_alpha(struct aic_de_comp *comp)
{
    struct aicfb_alpha_config *alpha = &comp->alpha[AICFB_LAYER_TYPE_UI];

    /* alpha has been configured */
    if (alpha->enable)
        return;

    /* default pixel alpha mode */
    alpha->layer_id = AICFB_LAYER_TYPE_UI;
    alpha->mode     = AICFB_PIXEL_ALPHA_MODE;
    alpha->enable   = 1;
    alpha->value    = 0x0;
}

static void aic_de_calc_config(const struct display_timing *timing)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    u32 vtotal = timing->vactive + timing->vfront_porch +
                 timing->vback_porch + timing->vsync_len;
    u32 htotal = timing->hactive + timing->hfront_porch +
                 timing->hback_porch + timing->hsync_len;
    u32 pixelclock = timing->pixelclock;

    u32 fps = pixelclock / htotal / vtotal;
    u32 frame_us = 1000000 / fps;
    u32 line_us = frame_us / vtotal;
    u32 line = 1;

    while (line_us < LAYER_CONFIG_TIME_US) {
        line_us = line_us << 1;
        line++;
    }

    comp->accum_line = vtotal - line - 1;
}

struct de_fmt_info {
    u32 fmt;
    u32 bpp;
};

static const struct de_fmt_info de_formats[] = {
    { .fmt = MPP_FMT_ARGB_8888, .bpp = 32, },
    { .fmt = MPP_FMT_ABGR_8888, .bpp = 32, },
    { .fmt = MPP_FMT_RGBA_8888, .bpp = 32, },
    { .fmt = MPP_FMT_BGRA_8888, .bpp = 32, },
    { .fmt = MPP_FMT_XRGB_8888, .bpp = 32, },
    { .fmt = MPP_FMT_XBGR_8888, .bpp = 32, },
    { .fmt = MPP_FMT_RGBX_8888, .bpp = 32, },
    { .fmt = MPP_FMT_BGRX_8888, .bpp = 32, },
    { .fmt = MPP_FMT_RGB_888,   .bpp = 24, },
    { .fmt = MPP_FMT_BGR_888,   .bpp = 24, },
    { .fmt = MPP_FMT_ARGB_1555, .bpp = 16, },
    { .fmt = MPP_FMT_ABGR_1555, .bpp = 16, },
    { .fmt = MPP_FMT_RGBA_5551, .bpp = 16, },
    { .fmt = MPP_FMT_BGRA_5551, .bpp = 16, },
    { .fmt = MPP_FMT_RGB_565,   .bpp = 16, },
    { .fmt = MPP_FMT_BGR_565,   .bpp = 16, },
    { .fmt = MPP_FMT_ARGB_4444, .bpp = 16, },
    { .fmt = MPP_FMT_ABGR_4444, .bpp = 16, },
    { .fmt = MPP_FMT_RGBA_4444, .bpp = 16, },
    { .fmt = MPP_FMT_BGRA_4444, .bpp = 16, },
};

static int aic_de_set_mode(struct aic_panel *panel, struct panel_desc *desc, u32 output_bpp, u32 dither_en)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    u32 source_bpp;
    int i;

    if (desc)
        comp->timing = desc->timings;
    else
        comp->timing = panel->timings;

    aic_de_calc_config(comp->timing);

    if (comp->timing->hactive > DE_DITHER_WIDTH_MAX) {
        memset(&comp->dither, 0x00, sizeof(struct aic_de_dither));
        pr_err("Screen width is invalid, disable dither\n");
        return -1;
    }

    for (i = 0; i < ARRAY_SIZE(de_formats); i++) {
        if (AICFB_FORMAT == de_formats[i].fmt)
            break;
    }
    source_bpp = de_formats[i].bpp;

    if (!dither_en || source_bpp < 24 || output_bpp == 24) {
        pr_debug("Disable dither, en(%d), source_bpp(%d), output_bpp(%d)\n",
                                 dither_en, source_bpp, output_bpp);
        memset(&comp->dither, 0x00, sizeof(struct aic_de_dither));
        return 0;
    }

    switch (output_bpp) {
    case 16:
        comp->dither.red_bitdepth = 5;
        comp->dither.gleen_bitdepth = 6;
        comp->dither.blue_bitdepth = 5;
        comp->dither.enable = 1;
        break;
    case 18:
        comp->dither.red_bitdepth = 6;
        comp->dither.gleen_bitdepth = 6;
        comp->dither.blue_bitdepth = 6;
        comp->dither.enable = 1;
        break;
    default:
        memset(&comp->dither, 0x00, sizeof(struct aic_de_dither));
        break;
    }

    return 0;
}

static int aic_de_clk_enable(void)
{
    hal_clk_enable(CLK_DE);
    hal_clk_enable(CLK_PIX);
    hal_clk_set_freq(CLK_DE, DE_FREQ);

    hal_reset_deassert(RESET_DE);
    return 0;
}

static int aic_de_clk_disable(void)
{
    hal_reset_assert(RESET_DE);

    hal_clk_disable(CLK_DE);
    hal_clk_disable(CLK_PIX);
    return 0;
}

static int aic_de_timing_enable(void)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    u32 active_w = comp->timing->hactive;
    u32 active_h = comp->timing->vactive;
    u32 hfp = comp->timing->hfront_porch;
    u32 hbp = comp->timing->hback_porch;
    u32 vfp = comp->timing->vfront_porch;
    u32 vbp = comp->timing->vback_porch;
    u32 hsync = comp->timing->hsync_len;
    u32 vsync = comp->timing->vsync_len;
    bool h_pol = !!(comp->timing->flags & DISPLAY_FLAGS_HSYNC_HIGH);
    bool v_pol = !!(comp->timing->flags & DISPLAY_FLAGS_VSYNC_HIGH);

    de_config_timing(comp->regs, active_w, active_h, hfp, hbp,
            vfp, vbp, hsync, vsync, h_pol, v_pol);

    de_set_blending_size(comp->regs, active_w, active_h);
    de_set_ui_layer_size(comp->regs, active_w, active_h, 0, 0);
    de_scaler0_active_handle(comp->regs, 0);

    de_config_prefetch_line_set(comp->regs, DE_PREFETCH_LINE);
    de_soft_reset_ctrl(comp->regs, 1);

    de_set_qos(comp->regs);

    de_set_mode(comp->regs, DE_MODE);
#if DE_AUTO_MODE
    if (de_set_te_pinmux(TE_PIN) < 0)
        pr_err("Invalid TE pin\n");

    de_set_te_pulse_width(comp->regs, TE_PULSE_WIDTH);
#endif

    if (comp->dither.enable)
        de_set_dither(comp->regs,
                comp->dither.red_bitdepth,
                comp->dither.gleen_bitdepth,
                comp->dither.blue_bitdepth,
                comp->dither.enable);

    de_check_ui_alpha(comp);
    de_ui_bg_blending_enable(comp->regs, 0);
    de_ui_alpha_blending_enable(comp->regs, comp->alpha[1].value,
                    comp->alpha[1].mode,
                    comp->alpha[1].enable);

    aicos_request_irq(DE_IRQn, aic_de_handler, 0, "aic-de", NULL);

#ifndef AIC_BOOTLOADER_CMD_PROGRESS_BAR
    de_timing_enable_interrupt(comp->regs, true, TIMING_INIT_V_BLANK_FLAG);
    de_timing_enable_interrupt(comp->regs, true, TIMING_INIT_UNDERFLOW_FLAG);
#ifdef AIC_DE_DRV_V11_1
    de_timing_enable_interrupt(comp->regs, true, TIMING_INIT_CFG_DONE);
#endif
#endif

    aic_de_set_gamma_config(&comp->gamma);
    aic_de_set_ccm_config(&comp->ccm);

    de_timing_enable(comp->regs, 1);

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_timing_disable(void)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_timing_enable(comp->regs, 0);
    aic_de_release_drvdata();
    return 0;
}

static int aic_de_wait_for_vsync(void)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    int ret = 0;

    ret = aicos_wqueue_wait(comp->vsync_queue, DE_TIMEOUT_MS);
    if (ret < 0) {
            hal_log_err("DE wait vsync irq failed, ret: %d\n", ret);
            return ret;
    }
    return 0;
}

static int aic_de_get_alpha_config(struct aicfb_alpha_config *alpha)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_support_alpha_blending(comp, alpha->layer_id) == false) {
        pr_err("layer %d doesn't support alpha blending\n",
            alpha->layer_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    comp->alpha[alpha->layer_id].layer_id = alpha->layer_id;
    memcpy(alpha, &comp->alpha[alpha->layer_id],
        sizeof(struct aicfb_alpha_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_update_alpha_config(struct aicfb_alpha_config *alpha)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_support_alpha_blending(comp, alpha->layer_id) == false) {
        pr_err("layer %d doesn't support alpha blending\n",
                alpha->layer_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    de_config_update_enable(comp->regs, 0);
    de_ui_alpha_blending_enable(comp->regs,
        alpha->value, alpha->mode, alpha->enable);
    de_config_update_enable(comp->regs, 1);
    memcpy(&comp->alpha[alpha->layer_id],
        alpha, sizeof(struct aicfb_alpha_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_ck_config(struct aicfb_ck_config *ck)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_support_color_key(comp, ck->layer_id) == false) {
        pr_err("Layer %d doesn't support color key blending\n", ck->layer_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }
    comp->ck[ck->layer_id].layer_id = ck->layer_id;
    memcpy(ck, &comp->ck[ck->layer_id], sizeof(struct aicfb_ck_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_update_ck_config(struct aicfb_ck_config *ck)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_support_color_key(comp, ck->layer_id) == false) {
        pr_err("Layer %d doesn't support color key blending\n", ck->layer_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    if (ck->enable > 1) {
        pr_err("Invalid ck enable: %d\n", ck->enable);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    de_config_update_enable(comp->regs, 0);
    de_ui_layer_color_key_enable(comp->regs, ck->value, ck->enable);
    de_config_update_enable(comp->regs, 1);
    memcpy(&comp->ck[ck->layer_id], ck, sizeof(struct aicfb_ck_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_layer_config(struct aicfb_layer_data *layer_data)
{
    u32 index;
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_valid_layer_and_rect_id(comp, layer_data->layer_id,
                       layer_data->rect_id) == false) {
        pr_err("invalid layer_id %d or rect_id %d\n",
            layer_data->layer_id, layer_data->rect_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    index = (layer_data->layer_id << RECT_NUM_SHIFT) + layer_data->rect_id;
    comp->layers[index].layer_id = layer_data->layer_id;
    comp->layers[index].rect_id = layer_data->rect_id;
    memcpy(layer_data, &comp->layers[index],
        sizeof(struct aicfb_layer_data));

    aic_de_release_drvdata();
    return 0;
}

static int update_one_layer_config(struct aic_de_comp *comp,
                   struct aicfb_layer_data *layer_data)
{
    u32 index;
    int ret;

    if (!is_valid_layer_and_rect_id(comp, layer_data->layer_id,
                       layer_data->rect_id)) {
        pr_err("%s() - layer_id %d or rect_id %d is invalid\n",
            __func__, layer_data->layer_id, layer_data->rect_id);
        return -EINVAL;
    }

    if (layer_data->enable == 0) {
        index = (layer_data->layer_id << RECT_NUM_SHIFT)
            + layer_data->rect_id;
        comp->layers[index].enable = 0;

        if (is_ui_layer(comp, layer_data->layer_id)) {
            ui_rect_disable(comp, layer_data->layer_id,
                    layer_data->rect_id);
        } else {
            de_video_layer_enable(comp->regs, 0);
            comp->layer_mask &= ~BIT(AICFB_LAYER_TYPE_VIDEO);
        }
        return 0;
    }

    if (is_ui_layer(comp, layer_data->layer_id)) {
        index = (layer_data->layer_id << RECT_NUM_SHIFT)
            + layer_data->rect_id;

        if (!is_valid_ui_rect_size(comp, layer_data))
            return -EINVAL;

        ret = config_ui_layer_rect(comp, layer_data);
        if (ret != 0)
            return -EINVAL;
        else
            memcpy(&comp->layers[index], layer_data,
                   sizeof(struct aicfb_layer_data));
    } else {
        index = layer_data->layer_id << RECT_NUM_SHIFT;

        if (!is_valid_video_size(comp, layer_data)) {
            comp->layers[index].enable = 0;
            de_video_layer_enable(comp->regs, 0);
            comp->layer_mask &= ~BIT(AICFB_LAYER_TYPE_VIDEO);
            return -EINVAL;
        }

        ret = config_video_layer(comp, layer_data);
        if (ret != 0) {
            comp->layers[index].enable = 0;
            de_video_layer_enable(comp->regs, 0);
            comp->layer_mask &= ~BIT(AICFB_LAYER_TYPE_VIDEO);
        } else {
            memcpy(&comp->layers[index], layer_data,
                   sizeof(struct aicfb_layer_data));
            comp->layer_mask |= BIT(AICFB_LAYER_TYPE_VIDEO);
        }
        return ret;
    }
    return 0;
}

static void aic_de_skip_prefetch_line(struct aic_de_comp *comp)
{
    u32 flags = BIT(AICFB_LAYER_TYPE_UI) | BIT(AICFB_LAYER_TYPE_VIDEO);
    u32 output_line = 0;
    u32 val;

    if ((comp->layer_mask & flags) != flags)
        /* Skip prefetch line only when both UI and VIDEO layers are enabled */
        return;

    output_line = reg_read(comp->regs + TIMING_DEBUG);
    if (output_line >= comp->accum_line || output_line <= DE_PREFETCH_LINE) {
        readl_poll_timeout(comp->regs + TIMING_DEBUG, val,
                           val > DE_PREFETCH_LINE, 100);
    }
}

static int aic_de_update_layer_config(struct aicfb_layer_data *layer_data)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    size_t flag;
    int ret;

    aic_de_skip_prefetch_line(comp);

    flag = aicos_enter_critical_section();

    de_config_update_enable(comp->regs, 0);
    ret = update_one_layer_config(comp, layer_data);
    de_config_update_enable(comp->regs, 1);

    aicos_leave_critical_section(flag);

    aic_de_release_drvdata();
    return ret;
}

static int aic_de_update_layer_config_list(struct aicfb_config_lists *list)
{
    int ret, i;
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_config_update_enable(comp->regs, 0);

    for (i = 0; i < list->num; i++) {
        if (is_ui_layer(comp, list->layers[i].layer_id)) {
            int index = (list->layers[i].layer_id << RECT_NUM_SHIFT)
                    + list->layers[i].rect_id;
            comp->layers[index].enable = 0;
        }
    }

    for (i = 0; i < list->num; i++) {
        ret = update_one_layer_config(comp, &list->layers[i]);
        if (ret)
            return ret;
    }
    de_config_update_enable(comp->regs, 1);

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_set_display_prop(struct aicfb_disp_prop *disp_prop)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_config_update_enable(comp->regs, 0);

    if (need_update_disp_prop(comp, disp_prop)) {
        /* get color space from video layer config */
        struct aicfb_layer_data *layer_data = &comp->layers[0];
        int color_space = MPP_BUF_COLOR_SPACE_GET(layer_data->buf.flags);

        de_update_csc(comp, disp_prop, color_space);
    }

    de_config_update_enable(comp->regs, 1);

    memcpy(&comp->disp_prop, disp_prop, sizeof(struct aicfb_disp_prop));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_display_prop(struct aicfb_disp_prop *disp_prop)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    memcpy(disp_prop, &comp->disp_prop, sizeof(struct aicfb_disp_prop));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_set_ccm_config(struct aicfb_ccm_config *ccm)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_config_update_enable(comp->regs, 0);

    if (ccm->enable) {
        de_config_ccm(comp->regs, ccm->ccm_table);
        de_ccm_ctrl(comp->regs, 1);
    } else {
        de_ccm_ctrl(comp->regs, 0);
    }

    memcpy(&comp->ccm, ccm, sizeof(struct aicfb_ccm_config));
    de_config_update_enable(comp->regs, 1);
    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_ccm_config(struct aicfb_ccm_config *ccm)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    memcpy(ccm, &comp->ccm, sizeof(struct aicfb_ccm_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_set_gamma_config(struct aicfb_gamma_config *gamma)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    int i;

    if (gamma->enable) {
        for (i = 0; i < 3; i++)
            de_config_gamma_lut(comp->regs, gamma->gamma_lut[i], i);

        de_gamma_ctrl(comp->regs, 1);
    } else {
        de_gamma_ctrl(comp->regs, 0);
    }

    memcpy(&comp->gamma, gamma, sizeof(struct aicfb_gamma_config));
    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_gamma_config(struct aicfb_gamma_config *gamma)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    memcpy(gamma, &comp->gamma, sizeof(struct aicfb_gamma_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_update_bg_blend_ui_config(u32 enable)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_ui_bg_blending_enable(comp->regs, enable);
    comp->bg_blend_ui_en = enable;

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_bg_blend_ui_config(u32 *enable)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    *enable = comp->bg_blend_ui_en;
    aic_de_release_drvdata();
    return 0;
}

struct de_funcs aic_de_funcs   = {
    .set_mode                  = aic_de_set_mode,
    .clk_enable                = aic_de_clk_enable,
    .clk_disable               = aic_de_clk_disable,
    .timing_enable             = aic_de_timing_enable,
    .timing_disable            = aic_de_timing_disable,
    .wait_for_vsync            = aic_de_wait_for_vsync,
    .get_alpha_config          = aic_de_get_alpha_config,
    .update_alpha_config       = aic_de_update_alpha_config,
    .get_ck_config             = aic_de_get_ck_config,
    .update_ck_config          = aic_de_update_ck_config,
    .get_layer_config          = aic_de_get_layer_config,
    .update_layer_config       = aic_de_update_layer_config,
    .update_layer_config_list  = aic_de_update_layer_config_list,
    .set_display_prop          = aic_de_set_display_prop,
    .get_display_prop          = aic_de_get_display_prop,
    .set_ccm_config            = aic_de_set_ccm_config,
    .get_ccm_config            = aic_de_get_ccm_config,
    .set_gamma_config          = aic_de_set_gamma_config,
    .get_gamma_config          = aic_de_get_gamma_config,
    .update_bg_blend_ui_config = aic_de_update_bg_blend_ui_config,
    .get_bg_blend_ui_config    = aic_de_get_bg_blend_ui_config,
};

static const struct aicfb_layer_num layer_num = {
    .vi_num = VI_LAYER_NUM,
    .ui_num = UI_LAYER_NUM,
};

static const struct aicfb_layer_capability aicfb_layer_cap[] = {
    {0, AICFB_LAYER_TYPE_VIDEO, VI_LAYER_WIDTH_MAX, VI_LAYER_HEIGHT_MAX, 0},
    {1, AICFB_LAYER_TYPE_UI,    UI_LAYER_WIDTH_MAX, UI_LAYER_HEIGHT_MAX,
     AICFB_CAP_4_RECT_WIN_FLAG | AICFB_CAP_ALPHA_FLAG | AICFB_CAP_CK_FLAG},
};

static const struct aic_de_configs aic_de_cfg = {
    .layer_num = &layer_num,
    .cap = aicfb_layer_cap,
};

static int aic_de_probe(void)
{
    struct aic_de_comp *comp;

    comp = aicos_malloc(0, sizeof(struct aic_de_comp));
    if (!comp)
    {
        pr_err("allloc de comp failed\n");
        return -ENOMEM;
    }

    memset(comp, 0, sizeof(*comp));

    comp->regs = (void *)DE_BASE;
    comp->config = &aic_de_cfg;
    comp->vsync_queue = aicos_wqueue_create();

#if defined AIC_DISPLAY_DITHER && !defined AIC_DISABLE_DITHER
    switch (AIC_DISP_OUTPUT_DEPTH)
    {
    case DITHER_RGB565:
        comp->dither.red_bitdepth = 5;
        comp->dither.gleen_bitdepth = 6;
        comp->dither.blue_bitdepth = 5;
        comp->dither.enable = 1;
        break;
    case DITHER_RGB666:
        comp->dither.red_bitdepth = 6;
        comp->dither.gleen_bitdepth = 6;
        comp->dither.blue_bitdepth = 6;
        comp->dither.enable = 1;
        break;
    default:
        pr_err("Invalid dither mode\n");
        break;
    }
#endif

    /* set display properties to default value */
    comp->disp_prop.bright = 50;
    comp->disp_prop.contrast = 50;
    comp->disp_prop.saturation = 50;
    comp->disp_prop.hue = 50;

#if defined(AIC_DE_DRV_V10) || defined(AIC_DE_V10)
    memset(&comp->ccm, 0x0, sizeof(struct aicfb_ccm_config));
    memset(&comp->gamma, 0x0, sizeof(struct aicfb_gamma_config));
#else
    comp->ccm.enable = 1;
    memcpy(&comp->ccm.ccm_table, ccm_lut, sizeof(ccm_lut));

    comp->gamma.enable = 1;
    memcpy(&comp->gamma.gamma_lut, gam_lut, sizeof(gam_lut));
#endif

    g_aic_de_comp = comp;

    return 0;
}

static void aic_de_remove(void)
{

}

struct platform_driver artinchip_de_driver = {
    .name = "artinchip-de",
    .component_type = AIC_DE_COM,
    .probe = aic_de_probe,
    .remove = aic_de_remove,
    .de_funcs = &aic_de_funcs,
};

