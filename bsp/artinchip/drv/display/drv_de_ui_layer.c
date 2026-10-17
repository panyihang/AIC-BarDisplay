/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drv_fb.h"
#include "drv_de.h"

bool is_ui_rect_win_overlap(struct aic_de_comp *comp,
                   u32 layer_id, u32 rect_id,
                   u32 x, u32 y, u32 w, u32 h)
{
    int i;
    u32 index;
    u32 cur_x, cur_y;
    u32 cur_w, cur_h;

    index = (layer_id << RECT_NUM_SHIFT);

    for (i = 0; i < MAX_RECT_NUM; i++) {
        if (rect_id == i) {
            index++;
            continue;
        }

        if (comp->layers[index].enable) {
            if (comp->layers[index].buf.crop_en) {
                cur_w = comp->layers[index].buf.crop.width;
                cur_h = comp->layers[index].buf.crop.height;
            } else {
                cur_w = comp->layers[index].buf.size.width;
                cur_h = comp->layers[index].buf.size.height;
            }
            cur_x = comp->layers[index].pos.x;
            cur_y = comp->layers[index].pos.y;

            if ((min(y + h, cur_y + cur_h) > max(y, cur_y)) &&
                (min(x + w, cur_x + cur_w) > max(x, cur_x))) {
                return true;
            }
        }
        index++;
    }
    return false;
}

bool is_valid_ui_rect_size(struct aic_de_comp *comp,
                  struct aicfb_layer_data *layer_data)
{
    u32 src_width;
    u32 src_height;
    u32 x_offset;
    u32 y_offset;
    u32 active_w;
    u32 active_h;

    u32 w;
    u32 h;

    src_width = layer_data->buf.size.width;
    src_height = layer_data->buf.size.height;
    x_offset = layer_data->pos.x;
    y_offset = layer_data->pos.y;
    active_w = comp->timing->hactive;
    active_h = comp->timing->vactive;

    if (x_offset >= active_w || y_offset >= active_h) {
        pr_err("ui layer x or y offset is invalid\n");
        return false;
    }

    if (layer_data->buf.crop_en) {
        u32 crop_x = layer_data->buf.crop.x;
        u32 crop_y = layer_data->buf.crop.y;

        if (crop_x >= src_width || crop_y >= src_height) {
            pr_err("ui layer crop is invalid\n");
            return false;
        }

        if ((crop_x + layer_data->buf.crop.width) > src_width)
            layer_data->buf.crop.width = src_width - crop_x;

        if ((crop_y + layer_data->buf.crop.height) > src_height)
            layer_data->buf.crop.height = src_height - crop_y;

        if ((x_offset + layer_data->buf.crop.width) > active_w)
            layer_data->buf.crop.width = active_w - x_offset;

        if ((y_offset + layer_data->buf.crop.height) > active_h)
            layer_data->buf.crop.height = active_h - y_offset;

        w = layer_data->buf.crop.width;
        h = layer_data->buf.crop.height;
    } else {
        if ((x_offset + src_width) > active_w)
            layer_data->buf.size.width = active_w - x_offset;

        if ((y_offset + src_height) > active_h)
            layer_data->buf.size.height = active_h - y_offset;

        w = layer_data->buf.size.width;
        h = layer_data->buf.size.height;
    }

    /* check overlap  */
    if (is_ui_rect_win_overlap(comp, layer_data->layer_id,
                   layer_data->rect_id,
                   x_offset, y_offset,    w, h)) {
        pr_err("ui rect is overlap\n");
        return false;
    }
    return true;
}

bool is_all_rect_win_disabled(struct aic_de_comp *comp,
                        u32 layer_id)
{
    int i;
    u32 index;

    index = (layer_id << RECT_NUM_SHIFT);

    for (i = 0; i < MAX_RECT_NUM; i++) {
        if (comp->layers[index].enable)
            return false;

        index++;
    }
    return true;
}

int ui_rect_disable(struct aic_de_comp *comp,
                  u32 layer_id, u32 rect_id)
{
    de_ui_layer_rect_enable(comp->regs, rect_id, 0);
    if (is_all_rect_win_disabled(comp, layer_id)) {
        de_ui_layer_enable(comp->regs, 0);
        comp->layer_mask &= ~BIT(AICFB_LAYER_TYPE_UI);
    }
    return 0;
}

int config_ui_layer_rect(struct aic_de_comp *comp,
                struct aicfb_layer_data *layer_data)
{
    enum mpp_pixel_format format = layer_data->buf.format;
    u32 in_w = (u32)layer_data->buf.size.width;
    u32 in_h = (u32)layer_data->buf.size.height;

    u32 stride0 = layer_data->buf.stride[0];
    u32 addr0 = layer_data->buf.phy_addr[0];
    u32 x_offset = layer_data->pos.x;
    u32 y_offset = layer_data->pos.y;
    u32 crop_en = layer_data->buf.crop_en;
    u32 crop_x = layer_data->buf.crop.x;
    u32 crop_y = layer_data->buf.crop.y;
    u32 crop_w = layer_data->buf.crop.width;
    u32 crop_h = layer_data->buf.crop.height;

    switch (format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
        if (crop_en) {
            addr0 += crop_x * 4 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
        if (crop_en) {
            addr0 += crop_x * 3 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
        if (crop_en) {
            addr0 += crop_x * 2 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    default:
        pr_err("invalid ui layer format: %d\n",
            format);

        return -EINVAL;
    }

    if (is_all_rect_win_disabled(comp, layer_data->layer_id)) {
        de_set_ui_layer_format(comp->regs, format);
        de_ui_layer_enable(comp->regs, 1);
        comp->layer_mask |= BIT(AICFB_LAYER_TYPE_UI);
    }

#ifdef AIC_SCREEN_CROP
    x_offset += AIC_SCREEN_CROP_POS_X;
    y_offset += AIC_SCREEN_CROP_POS_Y;
#endif

    de_ui_layer_set_rect(comp->regs, in_w, in_h, x_offset, y_offset,
                 stride0, addr0, layer_data->rect_id);
    de_ui_layer_rect_enable(comp->regs, layer_data->rect_id, 1);

    return 0;
}
