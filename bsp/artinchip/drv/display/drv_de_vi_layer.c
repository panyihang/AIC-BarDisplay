/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drv_fb.h"
#include "drv_de.h"

enum de_vi_format_flag {
    DE_RGB_FORMAT  = (0x1 << 0),

    DE_YUV_FORMAT = (0x1 << 1),
    DE_YUV_TILE   = (0x1 << 2),
    DE_YUV_PLANAR = (0x1 << 3),
    DE_YUV_PACKED = (0x1 << 4),
};

struct aic_de_vi_format_info {
    enum mpp_pixel_format format;
    unsigned int flags;
};

static struct aic_de_vi_format_info de_vi_layer_formats[] = {
    { MPP_FMT_ARGB_8888, DE_RGB_FORMAT },
    { MPP_FMT_ABGR_8888, DE_RGB_FORMAT },
    { MPP_FMT_RGBA_8888, DE_RGB_FORMAT },
    { MPP_FMT_BGRA_8888, DE_RGB_FORMAT },
    { MPP_FMT_XRGB_8888, DE_RGB_FORMAT },
    { MPP_FMT_XBGR_8888, DE_RGB_FORMAT },
    { MPP_FMT_RGBX_8888, DE_RGB_FORMAT },
    { MPP_FMT_BGRX_8888, DE_RGB_FORMAT },

    { MPP_FMT_RGB_888, DE_RGB_FORMAT },
    { MPP_FMT_BGR_888, DE_RGB_FORMAT },

    { MPP_FMT_ARGB_1555, DE_RGB_FORMAT },
    { MPP_FMT_ABGR_1555, DE_RGB_FORMAT },
    { MPP_FMT_RGBA_5551, DE_RGB_FORMAT },
    { MPP_FMT_BGRA_5551, DE_RGB_FORMAT },
    { MPP_FMT_RGB_565, DE_RGB_FORMAT },
    { MPP_FMT_BGR_565, DE_RGB_FORMAT },
    { MPP_FMT_ARGB_4444, DE_RGB_FORMAT },
    { MPP_FMT_ABGR_4444, DE_RGB_FORMAT },
    { MPP_FMT_RGBA_4444, DE_RGB_FORMAT },
    { MPP_FMT_BGRA_4444, DE_RGB_FORMAT },

    { MPP_FMT_YUV420P, DE_YUV_FORMAT | DE_YUV_PLANAR },
    { MPP_FMT_NV12, DE_YUV_FORMAT | DE_YUV_PLANAR },
    { MPP_FMT_NV21, DE_YUV_FORMAT | DE_YUV_PLANAR },
    { MPP_FMT_YUV422P, DE_YUV_FORMAT | DE_YUV_PLANAR },
    { MPP_FMT_NV16, DE_YUV_FORMAT | DE_YUV_PLANAR },
    { MPP_FMT_NV61, DE_YUV_FORMAT | DE_YUV_PLANAR },
    { MPP_FMT_YUYV, DE_YUV_FORMAT | DE_YUV_PACKED },
    { MPP_FMT_YVYU, DE_YUV_FORMAT | DE_YUV_PACKED },
    { MPP_FMT_UYVY, DE_YUV_FORMAT | DE_YUV_PACKED },
    { MPP_FMT_VYUY, DE_YUV_FORMAT | DE_YUV_PACKED },
    { MPP_FMT_YUV400, DE_YUV_FORMAT | DE_YUV_PLANAR },
    { MPP_FMT_YUV444P, DE_YUV_FORMAT | DE_YUV_PLANAR },

    { MPP_FMT_YUV420_64x32_TILE, DE_YUV_FORMAT | DE_YUV_TILE },
    { MPP_FMT_YUV420_128x16_TILE, DE_YUV_FORMAT | DE_YUV_TILE },
    { MPP_FMT_YUV422_64x32_TILE, DE_YUV_FORMAT | DE_YUV_TILE },
    { MPP_FMT_YUV422_128x16_TILE, DE_YUV_FORMAT | DE_YUV_TILE },

};

struct aic_de_vi_format_info * de_convert_vi_format(enum mpp_pixel_format format)
{
    struct aic_de_vi_format_info *format_info = de_vi_layer_formats;
    int i;

    for (i = 0; i < ARRAY_SIZE(de_vi_layer_formats); i++) {
        if (format_info->format == format)
            return format_info;

        format_info++;
    }

    return NULL;
}

static inline void de_check_scaler0_active(struct aic_de_comp *comp,
                                        u32 input_w, u32 input_h,
                                        u32 output_w, u32 output_h)
{
#if defined(AIC_DE_DRV_V10) || defined(AIC_DE_V10)
    int step = (input_h << 16) / output_h;
    u32 scaler_active = comp->scaler_active & 0xF;
    u32 index = 0;

    if (step <= 0x18000)
        index = 0;
    else if (step <= 0x20000)
        index = 1;
    else if (step <= 0x2C000)
        index = 2;
    else
        index = 3;

    if (scaler_active != index)
        comp->scaler_active = index | SCALER0_CTRL_ACTIVE;
#endif
}

bool is_valid_video_size(struct aic_de_comp *comp,
                struct aicfb_layer_data *layer_data)
{
    u32 src_width;
    u32 src_height;
    u32 x_offset;
    u32 y_offset;
    u32 active_w;
    u32 active_h;

    src_width = layer_data->buf.size.width;
    src_height = layer_data->buf.size.height;
    x_offset = layer_data->pos.x;
    y_offset = layer_data->pos.y;
    active_w = comp->timing->hactive;
    active_h = comp->timing->vactive;

    if (x_offset >= active_w || y_offset >= active_h) {
        pr_err("video layer x or y offset is invalid\n");
        return false;
    }

    if (layer_data->buf.crop_en) {
        u32 crop_x = layer_data->buf.crop.x;
        u32 crop_y = layer_data->buf.crop.y;

        if (crop_x >= src_width || crop_y >= src_height) {
            pr_err("video layer crop is invalid\n");
            return false;
        }

        if (crop_x + layer_data->buf.crop.width > src_width)
            layer_data->buf.crop.width = src_width - crop_x;

        if (crop_y + layer_data->buf.crop.height > src_height)
            layer_data->buf.crop.height = src_height - crop_y;
    }

    if (x_offset + layer_data->scale_size.width > active_w)
        layer_data->scale_size.width = active_w - x_offset;

    if (y_offset + layer_data->scale_size.height > active_h)
        layer_data->scale_size.height = active_h - y_offset;

    return true;
}

static int de_set_video_tile_format(struct aic_de_comp *comp, struct aicfb_layer_data *layer_data)
{
    enum mpp_pixel_format format = layer_data->buf.format;
    u32 in_w = (u32)layer_data->buf.size.width;
    u32 in_h = (u32)layer_data->buf.size.height;
    u32 in_w_ch1;
    u32 in_h_ch1;
    u32 stride0 = layer_data->buf.stride[0];
    u32 stride1 = layer_data->buf.stride[1];
    u32 addr0, addr1, addr2;
    u32 x_offset = layer_data->pos.x;
    u32 y_offset = layer_data->pos.y;
    u32 crop_en = layer_data->buf.crop_en;
    u32 crop_x = layer_data->buf.crop.x;
    u32 crop_y = layer_data->buf.crop.y;
    u32 crop_w = layer_data->buf.crop.width;
    u32 crop_h = layer_data->buf.crop.height;
    u32 tile_p0_x_offset = 0;
    u32 tile_p0_y_offset = 0;
    u32 tile_p1_x_offset = 0;
    u32 tile_p1_y_offset = 0;
    u32 scaler_w = layer_data->scale_size.width;
    u32 scaler_h = layer_data->scale_size.height;
    int color_space = MPP_BUF_COLOR_SPACE_GET(layer_data->buf.flags);

    addr0 = layer_data->buf.phy_addr[0];
    addr1 = layer_data->buf.phy_addr[1];
    addr2 = layer_data->buf.phy_addr[2];

    switch (format) {
    case MPP_FMT_YUV420_64x32_TILE:
        in_w = ALIGN_EVEN(in_w);
        in_h = ALIGN_EVEN(in_h);
        crop_x = ALIGN_EVEN(crop_x);
        crop_y = ALIGN_EVEN(crop_y);
        crop_w = ALIGN_EVEN(crop_w);
        crop_h = ALIGN_EVEN(crop_h);

        if (crop_en) {
            u32 tile_p0_x, tile_p0_y;
            u32 tile_p1_x, tile_p1_y;
            u32 offset_p0, offset_p1;

            tile_p0_x = crop_x >> 6;
            tile_p0_x_offset = crop_x & 63;
            tile_p0_y = crop_y >> 5;
            tile_p0_y_offset = crop_y & 31;

            tile_p1_x = crop_x >> 6;
            tile_p1_x_offset = crop_x & 63;
            tile_p1_y = (crop_y >> 1) >> 5;
            tile_p1_y_offset = (crop_y >> 1) & 31;

            offset_p0 = ALIGN_64B(stride0) * 32 * tile_p0_y
                    + 64 * 32 * tile_p0_x;

            offset_p1 = ALIGN_64B(stride1) * 32 * tile_p1_y
                    + 64 * 32 * tile_p1_x;

            addr0 += offset_p0;
            addr1 += offset_p1;

            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h >> 1;
        break;
    case MPP_FMT_YUV420_128x16_TILE:
        in_w = ALIGN_EVEN(in_w);
        in_h = ALIGN_EVEN(in_h);
        crop_x = ALIGN_EVEN(crop_x);
        crop_y = ALIGN_EVEN(crop_y);
        crop_w = ALIGN_EVEN(crop_w);
        crop_h = ALIGN_EVEN(crop_h);

        if (crop_en) {
            u32 tile_p0_x, tile_p0_y;
            u32 tile_p1_x, tile_p1_y;
            u32 offset_p0, offset_p1;

            tile_p0_x = crop_x >> 7;
            tile_p0_x_offset = crop_x & 127;

            tile_p0_y = crop_y >> 4;
            tile_p0_y_offset = crop_y & 15;

            tile_p1_x = crop_x >> 7;
            tile_p1_x_offset = crop_x & 127;

            tile_p1_y = (crop_y >> 1) >> 4;
            tile_p1_y_offset = (crop_y >> 1) & 15;

            offset_p0 = ALIGN_128B(stride0) * 16 * tile_p0_y
                    + 128 * 16 * tile_p0_x;

            offset_p1 = ALIGN_128B(stride1) * 16 * tile_p1_y
                    + 128 * 16 * tile_p1_x;

            addr0 += offset_p0;
            addr1 += offset_p1;

            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h >> 1;
        break;
    case MPP_FMT_YUV422_64x32_TILE:
        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            u32 tile_p0_x, tile_p0_y;
            u32 tile_p1_x, tile_p1_y;
            u32 offset_p0, offset_p1;

            tile_p0_x = crop_x >> 6;
            tile_p0_x_offset = crop_x & 63;
            tile_p0_y = crop_y >> 5;
            tile_p0_y_offset = crop_y & 31;

            tile_p1_x = crop_x >> 6;
            tile_p1_x_offset = crop_x & 63;
            tile_p1_y = crop_y >> 5;
            tile_p1_y_offset = crop_y & 31;

            offset_p0 = ALIGN_64B(stride0) * 32 * tile_p0_y
                    + 64 * 32 * tile_p0_x;

            offset_p1 = ALIGN_64B(stride1) * 32 * tile_p1_y
                    + 64 * 32 * tile_p1_x;

            addr0 += offset_p0;
            addr1 += offset_p1;
            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    case MPP_FMT_YUV422_128x16_TILE:
        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            u32 tile_p0_x, tile_p0_y;
            u32 tile_p1_x, tile_p1_y;
            u32 offset_p0, offset_p1;

            tile_p0_x = crop_x >> 7;
            tile_p0_x_offset = crop_x & 127;

            tile_p0_y = crop_y >> 4;
            tile_p0_y_offset = crop_y & 15;

            tile_p1_x = crop_x >> 7;
            tile_p1_x_offset = crop_x & 127;

            tile_p1_y = crop_y >> 4;
            tile_p1_y_offset = crop_y & 15;

            offset_p0 = ALIGN_128B(stride0) * 16 * tile_p0_y
                    + 128 * 16 * tile_p0_x;

            offset_p1 = ALIGN_128B(stride1) * 16 * tile_p1_y
                    + 128 * 16 * tile_p1_x;

            addr0 += offset_p0;
            addr1 += offset_p1;
            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    default:
        pr_err("invalid video layer format: %d, %s\n", format, __func__);

        return -EINVAL;
    }

    de_set_video_layer_info(comp->regs, in_w, in_h, format,
                stride0, stride1, addr0, addr1, addr2,
                x_offset, y_offset);

    de_set_video_layer_tile_offset(comp->regs,
                       tile_p0_x_offset, tile_p0_y_offset,
                       tile_p1_x_offset, tile_p1_y_offset);

    if (need_update_csc(comp, color_space)) {
        struct aicfb_disp_prop *disp_prop = &comp->disp_prop;

        de_update_csc(comp, disp_prop, color_space);
    }

    if (!scaler_w) {
        scaler_w = in_w;
        layer_data->scale_size.width = in_w;
    }

    if (!scaler_h) {
        scaler_h = in_h;
        layer_data->scale_size.height = in_h;
    }

    de_set_scaler0_channel(comp->regs, in_w, in_h,
                   scaler_w, scaler_h, 0);

    de_set_scaler0_channel(comp->regs,
                   in_w_ch1, in_h_ch1,
                   scaler_w, scaler_h, 1);

    de_check_scaler0_active(comp, in_w_ch1, in_h_ch1,
                   scaler_w, scaler_h);

    de_scaler0_enable(comp->regs, 1);

    de_video_layer_enable(comp->regs, 1);

    return 0;
}

static int de_set_video_rgb_format(struct aic_de_comp *comp, struct aicfb_layer_data *layer_data)
{
    enum mpp_pixel_format format = layer_data->buf.format;
    u32 in_w = (u32)layer_data->buf.size.width;
    u32 in_h = (u32)layer_data->buf.size.height;
    u32 stride0 = layer_data->buf.stride[0];
    u32 stride1 = layer_data->buf.stride[1];
    u32 addr0, addr1, addr2;
    u32 x_offset = layer_data->pos.x;
    u32 y_offset = layer_data->pos.y;
    u32 crop_en = layer_data->buf.crop_en;
    u32 crop_x = layer_data->buf.crop.x;
    u32 crop_y = layer_data->buf.crop.y;
    u32 crop_w = layer_data->buf.crop.width;
    u32 crop_h = layer_data->buf.crop.height;

    addr0 = layer_data->buf.phy_addr[0];
    addr1 = layer_data->buf.phy_addr[1];
    addr2 = layer_data->buf.phy_addr[2];

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
        pr_err("invalid video layer format: %d, %s\n", format, __func__);

        return -EINVAL;
    }

    de_set_video_layer_info(comp->regs, in_w, in_h, format,
                stride0, stride1, addr0, addr1, addr2,
                x_offset, y_offset);

    de_scaler0_enable(comp->regs, 0);

    de_video_layer_enable(comp->regs, 1);
    return 0;
}

static int de_set_video_planar_format(struct aic_de_comp *comp, struct aicfb_layer_data *layer_data)
{
    enum mpp_pixel_format format = layer_data->buf.format;
    u32 in_w = (u32)layer_data->buf.size.width;
    u32 in_h = (u32)layer_data->buf.size.height;
    u32 in_w_ch1;
    u32 in_h_ch1;
    u32 stride0 = layer_data->buf.stride[0];
    u32 stride1 = layer_data->buf.stride[1];
    u32 addr0, addr1, addr2;
    u32 x_offset = layer_data->pos.x;
    u32 y_offset = layer_data->pos.y;
    u32 crop_en = layer_data->buf.crop_en;
    u32 crop_x = layer_data->buf.crop.x;
    u32 crop_y = layer_data->buf.crop.y;
    u32 crop_w = layer_data->buf.crop.width;
    u32 crop_h = layer_data->buf.crop.height;
    u32 channel_num = 1;
    u32 scaler_en = 0;
    u32 scaler_w = layer_data->scale_size.width;
    u32 scaler_h = layer_data->scale_size.height;
    int color_space = MPP_BUF_COLOR_SPACE_GET(layer_data->buf.flags);

    addr0 = layer_data->buf.phy_addr[0];
    addr1 = layer_data->buf.phy_addr[1];
    addr2 = layer_data->buf.phy_addr[2];

    switch (format) {
    case MPP_FMT_YUV420P:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        in_h = ALIGN_EVEN(in_h);
        crop_x = ALIGN_EVEN(crop_x);
        crop_y = ALIGN_EVEN(crop_y);
        crop_w = ALIGN_EVEN(crop_w);
        crop_h = ALIGN_EVEN(crop_h);

        if (crop_en) {
            u32 ch1_offset;

            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;

            ch1_offset = (crop_x >> 1)
                     + (crop_y >> 1) * stride1;

            addr1 += ch1_offset;
            addr2 += ch1_offset;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h >> 1;
        break;
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        in_h = ALIGN_EVEN(in_h);
        crop_x = ALIGN_EVEN(crop_x);
        crop_y = ALIGN_EVEN(crop_y);
        crop_w = ALIGN_EVEN(crop_w);
        crop_h = ALIGN_EVEN(crop_h);

        if (crop_en) {
            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;

            addr1 += crop_x + (crop_y >> 1) * stride1;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h >> 1;

        break;
    case MPP_FMT_YUV400:
        channel_num = 1;
        scaler_en = 1;

        if (crop_en) {
            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    case MPP_FMT_YUV422P:
        channel_num = 2;
        scaler_en = 1;

        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            u32 ch1_offset;

            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;

            ch1_offset = (crop_x >> 1) + crop_y * stride1;
            addr1 += ch1_offset;
            addr2 += ch1_offset;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            u32 ch1_offset;

            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;

            ch1_offset = crop_x + crop_y * stride1;
            addr1 += ch1_offset;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    case MPP_FMT_YUV444P:
        channel_num = 2;
        scaler_en = 0;

        if (crop_en) {
            u32 ch1_offset;

            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
            ch1_offset = crop_x + crop_y * stride1;
            addr1 += ch1_offset;
            addr2 += ch1_offset;
        }

        break;
    default:
        pr_err("invalid video layer format: %d\n", format);

        return -EINVAL;
    }

    de_set_video_layer_info(comp->regs, in_w, in_h, format,
                stride0, stride1, addr0, addr1, addr2,
                x_offset, y_offset);

    if (scaler_en) {
        if (need_update_csc(comp, color_space)) {
            struct aicfb_disp_prop *disp_prop = &comp->disp_prop;

            de_update_csc(comp, disp_prop, color_space);
        }

        if (!scaler_w) {
            scaler_w = in_w;
            layer_data->scale_size.width = in_w;
        }

        if (!scaler_h) {
            scaler_h = in_h;
            layer_data->scale_size.height = in_h;
        }

        de_set_scaler0_channel(comp->regs, in_w, in_h,
                       scaler_w, scaler_h, 0);

        if (channel_num == 2) {
            de_set_scaler0_channel(comp->regs,
                           in_w_ch1, in_h_ch1,
                           scaler_w, scaler_h, 1);

            de_check_scaler0_active(comp, in_w_ch1, in_h_ch1,
                           scaler_w, scaler_h);
        }

        de_scaler0_enable(comp->regs, 1);
    } else {
        de_scaler0_enable(comp->regs, 0);
    }

    de_video_layer_enable(comp->regs, 1);
    return 0;
}

static int de_set_video_packed_format(struct aic_de_comp *comp, struct aicfb_layer_data *layer_data)
{
    enum mpp_pixel_format format = layer_data->buf.format;
    u32 in_w = (u32)layer_data->buf.size.width;
    u32 in_h = (u32)layer_data->buf.size.height;
    u32 in_w_ch1;
    u32 in_h_ch1;
    u32 stride0 = layer_data->buf.stride[0];
    u32 stride1 = layer_data->buf.stride[1];
    u32 addr0, addr1, addr2;
    u32 x_offset = layer_data->pos.x;
    u32 y_offset = layer_data->pos.y;
    u32 crop_en = layer_data->buf.crop_en;
    u32 crop_x = layer_data->buf.crop.x;
    u32 crop_y = layer_data->buf.crop.y;
    u32 crop_w = layer_data->buf.crop.width;
    u32 crop_h = layer_data->buf.crop.height;
    u32 scaler_w = layer_data->scale_size.width;
    u32 scaler_h = layer_data->scale_size.height;
    int color_space = MPP_BUF_COLOR_SPACE_GET(layer_data->buf.flags);

    addr0 = layer_data->buf.phy_addr[0];
    addr1 = layer_data->buf.phy_addr[1];
    addr2 = layer_data->buf.phy_addr[2];

    switch (format) {
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            addr0 += (crop_x << 1) + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    default:
        pr_err("invalid video layer format: %d, %s\n", format, __func__);

        return -EINVAL;
    }

    de_set_video_layer_info(comp->regs, in_w, in_h, format,
                stride0, stride1, addr0, addr1, addr2,
                x_offset, y_offset);

    if (need_update_csc(comp, color_space)) {
        struct aicfb_disp_prop *disp_prop = &comp->disp_prop;

        de_update_csc(comp, disp_prop, color_space);
    }

    if (!scaler_w) {
        scaler_w = in_w;
        layer_data->scale_size.width = in_w;
    }

    if (!scaler_h) {
        scaler_h = in_h;
        layer_data->scale_size.height = in_h;
    }

    de_set_scaler0_channel(comp->regs, in_w, in_h,
                   scaler_w, scaler_h, 0);

    de_set_scaler0_channel(comp->regs,
                   in_w_ch1, in_h_ch1,
                   scaler_w, scaler_h, 1);

    de_check_scaler0_active(comp, in_w_ch1, in_h_ch1,
                   scaler_w, scaler_h);

    de_scaler0_enable(comp->regs, 1);

    de_video_layer_enable(comp->regs, 1);

    return 0;
}

static int de_vi_layer_config_format(struct aic_de_comp *comp,
                struct aic_de_vi_format_info *format_info,
                struct aicfb_layer_data *layer_data)
{
    if (format_info->flags & DE_RGB_FORMAT)
        return de_set_video_rgb_format(comp, layer_data);

    if (format_info->flags & DE_YUV_TILE)
        return de_set_video_tile_format(comp, layer_data);

    if (format_info->flags & DE_YUV_PLANAR)
        return de_set_video_planar_format(comp, layer_data);

    if (format_info->flags & DE_YUV_PACKED)
        return de_set_video_packed_format(comp, layer_data);

    pr_err("invalid video layer format: %d, %s\n",
            layer_data->buf.format, __func__);
    return -EINVAL;
}

static void de_vi_layer_config_crop_pos(struct aic_de_comp *comp,
                          struct aicfb_layer_data *layer_data)
{
#ifdef AIC_SCREEN_CROP
    u32 x_offset = layer_data->pos.x + AIC_SCREEN_CROP_POS_X;
    u32 y_offset = layer_data->pos.y + AIC_SCREEN_CROP_POS_Y;

    reg_write(comp->regs + VIDEO_LAYER_OFFSET,
          VIDEO_LAYER_OFFSET_SET(x_offset, y_offset));
#endif
}

int config_video_layer(struct aic_de_comp *comp,
                  struct aicfb_layer_data *layer_data)
{
    enum mpp_pixel_format format = layer_data->buf.format;
    struct aic_de_vi_format_info *format_info;

    if (layer_data->buf.buf_type != MPP_PHY_ADDR) {
        pr_err("invalid buf type: %d\n", layer_data->buf.buf_type);
        return -EINVAL;
    };

    format_info = de_convert_vi_format(format);
    if (!format_info) {
        pr_err("invalid video layer format: %d, %s\n", format, __func__);
        return -EINVAL;
    }

    de_vi_layer_config_format(comp, format_info, layer_data);
    de_vi_layer_config_crop_pos(comp, layer_data);

    return 0;
}
