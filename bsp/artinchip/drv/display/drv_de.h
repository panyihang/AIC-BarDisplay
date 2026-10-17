/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DRV_DE_H
#define _DRV_DE_H_

#include <aic_hal_de.h>
#include <aic_drv_de.h>

#define MAX_LAYER_NUM 2
#define MAX_RECT_NUM 4
#define RECT_NUM_SHIFT 2
#define CONFIG_NUM   (MAX_LAYER_NUM * MAX_RECT_NUM)

struct aic_de_dither {
    unsigned int enable;
    unsigned int red_bitdepth;
    unsigned int gleen_bitdepth;
    unsigned int blue_bitdepth;
};

struct aic_de_configs {
    const struct aicfb_layer_num *layer_num;
    const struct aicfb_layer_capability *cap;
};

struct aic_de_comp {
    void *regs;

    const struct aic_de_configs *config;

    struct aic_de_dither dither;
    struct aicfb_layer_data layers[CONFIG_NUM];
    struct aicfb_alpha_config alpha[MAX_LAYER_NUM];
    struct aicfb_ck_config ck[MAX_LAYER_NUM];
    struct aicfb_disp_prop disp_prop;
    struct aicfb_ccm_config ccm;
    struct aicfb_gamma_config gamma;

    const struct display_timing *timing;
    aicos_wqueue_t vsync_queue;
    unsigned int bg_blend_ui_en;
    unsigned int accum_line;
    u32 layer_mask;
    u32 scaler_active;

    /* INTERRUPT Context */
    de_vsync_cb cb;
    void *cb_data;
};

/**
 * Video Layer (VI) function
 */
bool is_valid_video_size(struct aic_de_comp *comp,
                struct aicfb_layer_data *layer_data);

int config_video_layer(struct aic_de_comp *comp,
                  struct aicfb_layer_data *layer_data);

/**
 * UI Layer function
 */
int config_ui_layer_rect(struct aic_de_comp *comp,
                struct aicfb_layer_data *layer_data);

bool is_valid_ui_rect_size(struct aic_de_comp *comp,
                  struct aicfb_layer_data *layer_data);

bool is_ui_rect_win_overlap(struct aic_de_comp *comp,
                   u32 layer_id, u32 rect_id,
                   u32 x, u32 y, u32 w, u32 h);

bool is_all_rect_win_disabled(struct aic_de_comp *comp,
                        u32 layer_id);

int ui_rect_disable(struct aic_de_comp *comp,
                  u32 layer_id, u32 rect_id);

static inline bool is_ui_layer(struct aic_de_comp *comp, u32 layer_id)
{
    if (comp->config->cap[layer_id].layer_type == AICFB_LAYER_TYPE_UI)
        return true;
    else
        return false;
}

/**
 * hsbc function
 */
static inline bool need_use_hsbc(struct aic_de_comp *comp,
                 struct aicfb_disp_prop *disp_prop)
{
    if (disp_prop->bright != 50 ||
        disp_prop->contrast != 50 ||
        disp_prop->saturation != 50 ||
        disp_prop->hue != 50)
        return true;

    return false;
}

/**
 * csc function
 */
static inline bool need_update_csc(struct aic_de_comp *comp, int color_space)
{
    /* get color space from video layer config */
    struct aicfb_layer_data *layer_data = &comp->layers[0];

    if (color_space != MPP_BUF_COLOR_SPACE_GET(layer_data->buf.flags))
        return true;

    return false;
}

static inline int de_update_csc(struct aic_de_comp *comp,
                     struct aicfb_disp_prop *disp_prop,
                     int color_space)
{
    if (need_use_hsbc(comp, disp_prop)) {
        int bright = (int)disp_prop->bright;
        int contrast = (int)disp_prop->contrast;
        int saturation = (int)disp_prop->saturation;
        int hue = (int)disp_prop->hue;

        de_set_hsbc_with_csc_coefs(comp->regs, color_space,
                       bright, contrast,
                       saturation, hue);
    } else {
        de_set_csc0_coefs(comp->regs, color_space);
    }

    return 0;
}


#endif /* _DRV_DE_H_ */
