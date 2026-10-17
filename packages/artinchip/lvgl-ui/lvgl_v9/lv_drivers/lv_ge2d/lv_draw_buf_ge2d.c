/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <aic_core.h>
#include "lv_draw_ge2d.h"

#if LV_USE_DRAW_GE2D

static void ge2d_buf_invalidate_cache_cb(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
    const lv_image_header_t *header = &draw_buf->header;
    uint32_t stride = header->stride;
    lv_color_format_t cf = header->cf;
    uint32_t bpp = lv_color_format_get_size(cf);
    int32_t width = lv_area_get_width(area);
    int32_t height = lv_area_get_height(area);
    uint8_t *address = draw_buf->data;
    int32_t flush_line_size = (int32_t)width * (int32_t)bpp;

    address = address + (area->x1 * (int32_t)bpp) + (stride * (uint32_t)area->y1);

    int32_t i = 0;
    for(i = 0; i < height; i++) {
        int32_t size_shift = (int32_t)(((ulong)address) & (CACHE_LINE_SIZE  - 1));
        aicos_dcache_clean_invalid_range((void *)ALIGN_DOWN((ulong)address, CACHE_LINE_SIZE),
                                         (u32)ALIGN_UP(flush_line_size + size_shift, CACHE_LINE_SIZE));
        address += stride;
    }
}

void lv_draw_buf_ge2d_init_handlers(void)
{
    lv_draw_buf_handlers_t * handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb = ge2d_buf_invalidate_cache_cb;
}

#endif /*LV_USE_DRAW_GE2D*/
