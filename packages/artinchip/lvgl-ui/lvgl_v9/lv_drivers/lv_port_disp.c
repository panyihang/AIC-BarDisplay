/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <aic_core.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#ifdef LPKG_MPP
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_tpc_run.h"
#include "lv_draw_ge2d.h"
#include "lv_mpp_dec.h"

#if LV_DISP_FB_DUMP
static char g_save_path[128] = { 0 };
static bool g_dump_flag = false;

static inline void lv_disp_buf_dump(lv_display_t *disp)
{
    if (g_dump_flag) {
        g_dump_flag = false;
        lv_fs_res_t res = LV_FS_RES_OK;
        lv_color_format_t cf = lv_display_get_color_format(disp);
        uint32_t h = lv_display_get_vertical_resolution(disp);
        uint32_t w = lv_display_get_horizontal_resolution(disp);
        uint32_t stride = lv_draw_buf_width_to_stride(w, cf);
        lv_draw_buf_t *disp_buf = lv_display_get_buf_active(disp);
        lv_fs_file_t file_save;
        uint32_t data_write = 0;
        char src[128];

        snprintf(src, 127, "L:%s", g_save_path);
        res = lv_fs_open(&file_save, src, LV_FS_MODE_WR);
        if(res != LV_FS_RES_OK) {
            LV_LOG_ERROR("open %s failed", src);
            return;
        }
        lv_fs_write(&file_save, disp_buf->data, h * stride, &data_write);
        lv_fs_close(&file_save);
        printf("save %s ok\n", g_save_path);
    }
}

static void lv_fb_dump(int argc, char **argv)
{
    int ret;
    const char sopts[] = "u:o:";
    const struct option lopts[] = {
        {"usage",   no_argument, NULL, 'u'},
        {"output",  required_argument, NULL, 'o'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'o':
            strcpy(g_save_path, optarg);
            break;
        case 'u':
            printf("Usage: %s [Options]: \n", argv[0]);
            printf("\t-o, --output  image path\n");
            printf("\tfor example:\n");
            printf("\tlv_fb_dump -o /sdcard/out.bin\n");
            return;
        default:
            LV_LOG_ERROR("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    if (strlen(g_save_path)) {
        g_dump_flag = true;
    } else {
        printf("please check output path:%s\n", g_save_path);
    }
    return;
}

MSH_CMD_EXPORT_ALIAS(lv_fb_dump, lv_dump, lvgl dump display buffer);
#endif

#ifndef LV_USE_SPI_REPLACE_MIPI_DBI
static void display_cal_frame_rate(aic_disp_t *aic_disp)
{
#if DISP_SHOW_FPS == 1
    static int start_cal = 0;
    static int frame_cnt = 0;
    static u64 start_us, end_us;
    int interval = 1000 * 1000;
    int time_diff = 0;

    if (start_cal == 0) {
        start_cal = 1;
        start_us = (long)aic_get_time_us();
    }

    end_us = (long)aic_get_time_us();
    time_diff = (int)(end_us - start_us);
    if (time_diff >= interval) {
        aic_disp->fps = frame_cnt;
        frame_cnt = 0;
        start_cal = 0;
    } else {
        frame_cnt++;
    }
#else
    (void)aic_disp;
#endif
    return;
}

static inline void *get_fb_buf_by_id(aic_disp_t *aic_disp, int id)
{
    if (id == 1)
        return (void *)(aic_disp->info.framebuffer + aic_disp->fb_size);
    else
        return (void *)aic_disp->info.framebuffer;
}

#if LV_USE_OS && defined(AIC_PAN_DISPLAY)
static inline void wait_sync_ready(aic_disp_t *aic_disp)
{
    while (aic_disp->sync_ready == false) {
        if (aic_disp->exit_status)
            break;

        lv_thread_sync_wait(&aic_disp->sync_notify);
    }
}
#endif

static inline void disp_do_blit(aic_disp_t *aic_disp, lv_display_t *disp, lv_draw_buf_t *disp_buf)
{
    void *dest_buf = get_fb_buf_by_id(aic_disp, aic_disp->buf_id);
    int32_t hor_res = lv_display_get_horizontal_resolution(disp);
    int32_t ver_res = lv_display_get_vertical_resolution(disp);
    lv_color_format_t cf = lv_display_get_color_format(disp);
    int32_t src_stride = lv_draw_buf_width_to_stride(hor_res, cf);
    int32_t dst_stride = (int32_t)aic_disp->info.stride;
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

#if LV_USE_DRAW_GE2D
    lv_draw_ge2d_rotate(disp_buf->data, dest_buf, hor_res, ver_res,
                        src_stride, dst_stride, rotation, cf);
#else
    lv_draw_sw_rotate(disp_buf->data, dest_buf, hor_res, ver_res,
                      src_stride, dst_stride, rotation, cf);
#endif
}

static inline void display_power_on(aic_disp_t *aic_disp)
{
#ifndef AIC_DISP_COLOR_BLOCK
    static bool first_frame = true;
    if (first_frame) {
        mpp_fb_ioctl(aic_disp->fb, AICFB_POWERON, 0);
        first_frame = false;
    }
#else
    (void)aic_disp;
#endif
}

static inline void disp_draw_buf_flush(aic_disp_t *aic_disp, lv_display_t *disp, lv_draw_buf_t *disp_buf)
{
#if LV_USE_OS && defined(AIC_PAN_DISPLAY)
    wait_sync_ready(aic_disp);
    aic_disp->sync_ready = false;
    disp_do_blit(aic_disp, disp, disp_buf);
    mpp_fb_ioctl(aic_disp->fb, AICFB_PAN_DISPLAY , &aic_disp->buf_id);

    display_power_on(aic_disp);
    aic_disp->flush_act = true;
    lv_thread_sync_signal(&aic_disp->sync);
#else
#ifdef AIC_PAN_DISPLAY
    disp_do_blit(aic_disp, disp, disp_buf);
    mpp_fb_ioctl(aic_disp->fb, AICFB_PAN_DISPLAY , &aic_disp->buf_id);

    display_power_on(aic_disp);
    mpp_fb_ioctl(aic_disp->fb, AICFB_WAIT_FOR_VSYNC, 0);
#else
    display_power_on(aic_disp);
    mpp_fb_ioctl(aic_disp->fb, AICFB_WAIT_FOR_VSYNC, 0);
    disp_do_blit(aic_disp, disp, disp_buf);
#endif // AIC_PAN_DISPLAY
#endif
}

#if defined(LV_DISPLAY_ROTATE_GE_PARTIAL) && defined(LV_ROTATE_DEGREE) && \
    (LV_ROTATE_DEGREE == 90)
static int disp_rotate_partial_ge(aic_disp_t *aic_disp, lv_display_t *disp,
                                 const lv_area_t *area, const void *pixels);
#endif

static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    aic_disp_t *aic_disp = (aic_disp_t *)lv_display_get_user_data(disp);

    lv_draw_buf_t *disp_buf = lv_display_get_buf_active(disp);

#if defined(LV_DISPLAY_ROTATE_GE_PARTIAL) && defined(LV_ROTATE_DEGREE) && \
    (LV_ROTATE_DEGREE == 90)
    if (disp_buf->data == aic_disp->buf_aligned) {
        if (disp_rotate_partial_ge(aic_disp, disp, area, px_map) < 0)
            LV_LOG_ERROR("GE partial rotate failed area=%ld,%ld-%ld,%ld",
                         (long)area->x1, (long)area->y1,
                         (long)area->x2, (long)area->y2);
        display_power_on(aic_disp);
        if (lv_disp_flush_is_last(disp))
            display_cal_frame_rate(aic_disp);
        lv_display_flush_ready(disp);
        return;
    }
#else
    (void)px_map;
#endif

    if (lv_disp_flush_is_last(disp)) {
#if LV_DISP_FB_DUMP
        // dump frame buffer
        lv_disp_buf_dump(disp);
#endif
        if (disp_buf->data == aic_disp->buf_aligned) {
#ifdef AIC_PAN_DISPLAY
            aic_disp->buf_id  =  aic_disp->buf_id > 0 ? 0 : 1;
#else
            aic_disp->buf_id = 0;
#endif
            // flush cache
            aicos_dcache_clean_invalid_range((ulong *)disp_buf->data, (ulong)ALIGN_UP(aic_disp->buf_size, CACHE_LINE_SIZE));
            disp_draw_buf_flush(aic_disp, disp, disp_buf);
        } else {
            // flush cache
            aicos_dcache_clean_invalid_range((ulong *)disp_buf->data, (ulong)ALIGN_UP(aic_disp->fb_size, CACHE_LINE_SIZE));
#ifdef AIC_PAN_DISPLAY
            int index;
            if(disp_buf->data == aic_disp->info.framebuffer) {
                index = 0;
            } else {
                index = 1;
            }

            // send framebuffer to display
            mpp_fb_ioctl(aic_disp->fb, AICFB_PAN_DISPLAY , &index);
            display_power_on(aic_disp);
            // wait vsync
            mpp_fb_ioctl(aic_disp->fb, AICFB_WAIT_FOR_VSYNC, 0);
#endif
        }

        display_cal_frame_rate(aic_disp);
    }

    lv_display_flush_ready(disp);
}

static lv_color_format_t lv_display_fmt(enum mpp_pixel_format cf)
{
    lv_color_format_t fmt = LV_COLOR_FORMAT_ARGB8888;
    switch(cf) {
        case MPP_FMT_RGB_565:
            fmt = LV_COLOR_FORMAT_RGB565;
            break;
        case MPP_FMT_RGB_888:
            fmt = LV_COLOR_FORMAT_RGB888;
            break;
        case MPP_FMT_ARGB_8888:
            fmt = LV_COLOR_FORMAT_ARGB8888;
            break;
        case MPP_FMT_XRGB_8888:
            fmt = LV_COLOR_FORMAT_XRGB8888;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", cf);
            break;
    }
    return fmt;
}

#if LV_USE_OS && defined(AIC_PAN_DISPLAY) && defined(LV_DISPLAY_ROTATE_EN)
static void aic_display_thread(void *ptr)
{
    aic_disp_t *aic_disp = (aic_disp_t *)ptr;

    while(1) {
        while (aic_disp->flush_act == false) {
            if(aic_disp->exit_status)
                break;

            lv_thread_sync_wait(&aic_disp->sync);
        }

        aic_disp->flush_act = false;
        mpp_fb_ioctl(aic_disp->fb, AICFB_WAIT_FOR_VSYNC, 0);
        aic_disp->sync_ready = true;
        lv_thread_sync_signal(&aic_disp->sync_notify);

        if (aic_disp->exit_status) {
            LV_LOG_INFO("Ready to exit aic display thread.");
            break;
        }
    }

    lv_thread_sync_delete(&aic_disp->sync);
    lv_thread_sync_delete(&aic_disp->sync_notify);
    LV_LOG_INFO("Exit aic display thread.");
}
#endif

#if defined(LV_DISPLAY_ROTATE_EN)
#define LV_ROTATE_PARTIAL_LINES 32
static uint8_t *create_draw_buf(aic_disp_t *aic_disp, int w, int h, lv_color_format_t cf)
{
    int bpp = lv_color_format_get_bpp(cf) / 8;
    int buf_size;

#if defined(LV_DISPLAY_ROTATE_GE_PARTIAL)
    /* Rotation is accelerated per invalidated rectangle. Keep only a small
     * number of logical rows in CMA instead of a full-screen source image. */
    (void)h;
    buf_size = ALIGN_UP(w, 8) * LV_ROTATE_PARTIAL_LINES * bpp;
#else
    buf_size = ALIGN_UP(w, 8) * ALIGN_UP(h, 8) * bpp;
#endif

    aic_disp->buf = (uint8_t *)aicos_malloc_try_cma(buf_size + CACHE_LINE_SIZE - 1);
    if (!aic_disp->buf) {
        return NULL;
    } else {
        unsigned int align_addr = (unsigned int)ALIGN_UP((ulong) aic_disp->buf, CACHE_LINE_SIZE);
        aic_disp->buf_aligned = (uint8_t *)(ulong)align_addr;
        aicos_dcache_clean_invalid_range((ulong *)(ulong)align_addr, ALIGN_UP(buf_size, CACHE_LINE_SIZE));
    }

    aic_disp->buf_size = ALIGN_UP(buf_size, CACHE_LINE_SIZE);
    return aic_disp->buf_aligned;
}
#endif

#if defined(LV_DISPLAY_ROTATE_GE_PARTIAL) && defined(LV_ROTATE_DEGREE) && \
    (LV_ROTATE_DEGREE == 90)
static int disp_rotate_partial_ge(aic_disp_t *aic_disp, lv_display_t *disp,
                                 const lv_area_t *area, const void *pixels)
{
    struct ge_bitblt blt = {0};
    struct mpp_ge *ge = get_ge2d_device();
    int32_t width = lv_area_get_width(area);
    int32_t height = lv_area_get_height(area);
    int32_t physical_height = (int32_t)aic_disp->info.height;
    int32_t physical_x = area->y1;
    int32_t physical_y = physical_height - area->x2 - 1;
    uint32_t bpp = lv_color_format_get_size(lv_display_get_color_format(disp));
    lv_draw_buf_t *draw_buf = lv_display_get_buf_active(disp);
    uint32_t src_stride;

    if (!ge || !draw_buf || width <= 0 || height <= 0 ||
        physical_x < 0 || physical_y < 0 ||
        physical_x + height > aic_disp->info.width ||
        physical_y + width > aic_disp->info.height || bpp == 0)
        return -1;

    /* LVGL reshapes the partial buffer to the current invalidated width.
     * Consequently narrow areas can legally contain more than the nominal
     * 32 rows, while the full-width case is still limited by buf_size. */
    src_stride = draw_buf->header.stride;
    if (src_stride == 0 || (uint64_t)src_stride * height > draw_buf->data_size)
        return -1;

    aicos_dcache_clean_invalid_range((ulong *)pixels,
                                    src_stride * height);
    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = (uintptr_t)pixels;
    blt.src_buf.stride[0] = src_stride;
    blt.src_buf.size.width = width;
    blt.src_buf.size.height = height;
    blt.src_buf.format = lv_display_fmt(aic_disp->info.format);

    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    /* Use a patch-sized destination and offset its base address to the
     * physical top-left corner.  GE rotation with a full-surface destination
     * crop is unreliable on D12P for RGB888, while the same operation works
     * consistently when the destination is expressed as an independent block.
     */
    blt.dst_buf.phy_addr[0] = (uintptr_t)aic_disp->info.framebuffer +
                              (uintptr_t)physical_y * aic_disp->info.stride +
                              (uintptr_t)physical_x * bpp;
    blt.dst_buf.stride[0] = aic_disp->info.stride;
    blt.dst_buf.size.width = height;
    blt.dst_buf.size.height = width;
    blt.dst_buf.format = lv_display_fmt(aic_disp->info.format);
    /* LVGL 90 degrees is counterclockwise; the matching GE transform is 270. */
    blt.ctrl.flags = MPP_ROTATION_270;

    if (mpp_ge_bitblt(ge, &blt) < 0 || mpp_ge_emit(ge) < 0 || mpp_ge_sync(ge) < 0)
        return -1;
    return 0;
}
#endif

static int lv_de_display_init()
{
    void *buf1;
    void *buf2;
    aic_disp_t *aic_disp;
    int fb_size;
    int width;
    int height;
    int stride;
    lv_color_format_t cf;

    aic_disp = (aic_disp_t *)lv_malloc_zeroed(sizeof(aic_disp_t));
    if (!aic_disp) {
        LV_LOG_ERROR("malloc aic display failed");
        return -1;
    }

    aic_disp->fb = mpp_fb_open();
    if (aic_disp->fb) {
        mpp_fb_ioctl(aic_disp->fb, AICFB_GET_SCREENINFO, &aic_disp->info);
        width = aic_disp->info.width;
        height = aic_disp->info.height;
        stride = aic_disp->info.stride;
        fb_size = height * stride;
        aic_disp->fb_size = fb_size;

        /* The rotation path writes only invalidated rectangles.  Start from
         * a deterministic framebuffer so an area missed by the first LVGL
         * refresh cannot expose stale PSRAM contents as display noise. */
        memset(aic_disp->info.framebuffer, 0, fb_size);
        aicos_dcache_clean_range((ulong *)aic_disp->info.framebuffer, fb_size);

#ifdef AIC_PAN_DISPLAY
        buf1 = (void *)(aic_disp->info.framebuffer + fb_size);
        buf2 = (void *)aic_disp->info.framebuffer;
#else
        buf1 = (void *)aic_disp->info.framebuffer;
        buf2 = NULL;
#endif
    } else {
        LV_LOG_ERROR("get device fb info failed");
        lv_free(aic_disp);
        return -1;
    }

    cf = lv_display_fmt(aic_disp->info.format);
#if defined(LV_DISPLAY_ROTATE_EN)
#if defined(LV_DISPLAY_ROTATE_GE_PARTIAL)
    if (!create_draw_buf(aic_disp, height, width, cf)) {
#else
    if (!create_draw_buf(aic_disp, height, width, cf)) {
#endif
        LV_LOG_ERROR("create draw buffer failed");
        mpp_fb_close(aic_disp->fb);
        lv_free(aic_disp);
        return -1;
    } else {
        buf1 = (void *)aic_disp->buf_aligned;
        buf2 = NULL;
        aic_disp->buf_id = 0;
    }
#endif

    lv_display_t *disp = lv_display_create(width, height);
    lv_display_set_color_format(disp, cf);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_user_data(disp, aic_disp);

#if defined(LV_DISPLAY_ROTATE_EN) && defined(LV_ROTATE_DEGREE)
    lv_display_set_rotation(disp, LV_ROTATE_DEGREE / 90);
#endif

#if defined(LV_DISPLAY_ROTATE_GE_PARTIAL) && defined(LV_ROTATE_DEGREE) && \
    (LV_ROTATE_DEGREE == 90)
    (void)buf2;
    lv_display_set_buffers(disp, buf1, NULL, aic_disp->buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    /* set_rotation() resets LVGL's invalidation list.  Re-invalidate the
     * active screen after the logical resolution has been swapped so the
     * complete rotated canvas is submitted once. */
    lv_obj_invalidate(lv_display_get_screen_active(disp));
#elif defined(LV_DISPLAY_ROTATE_EN) && defined(LV_ROTATE_DEGREE)
    lv_display_set_buffers(disp, buf1, buf2, aic_disp->buf_size, LV_DISPLAY_RENDER_MODE_DIRECT);
#else
    lv_display_set_buffers(disp, buf1, buf2, fb_size, LV_DISPLAY_RENDER_MODE_DIRECT);
#endif

#if LV_USE_OS && defined(AIC_PAN_DISPLAY) && defined(LV_DISPLAY_ROTATE_EN)
    aic_disp->sync_ready = true;
    lv_thread_sync_init(&aic_disp->sync);
    lv_thread_sync_init(&aic_disp->sync_notify);
    lv_thread_init(&aic_disp->thread, LV_THREAD_PRIO_HIGH, aic_display_thread, 4 * 1024, aic_disp);
#endif
    return 0;
}
#endif /* LV_USE_SPI_REPLACE_MIPI_DBI */

static int lv_port_disp_info(struct aicfb_screeninfo *fb_info)
{
    struct mpp_fb *fb = mpp_fb_open();
    if (!fb) {
        LV_LOG_ERROR("get device fb info failed");
        return -1;
    }

    if (mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, fb_info) < 0) {
        LV_LOG_ERROR("get device fb info failed");
        return -1;
    }
    mpp_fb_close(fb);
    return 0;
}

void lv_port_disp_init(void)
{
    struct aicfb_screeninfo fb_info = {0};

#if LV_USE_MPP_DEC
    lv_mpp_dec_init();
#endif

#if LV_USE_DRAW_GE2D
    lv_draw_ge2d_init();
#endif

    lv_port_disp_info(&fb_info);

#ifndef LV_USE_SPI_REPLACE_MIPI_DBI
    if (lv_de_display_init() < 0) {
        LV_LOG_ERROR("init de display failed");
        return;
    }
#endif

#if defined(AIC_LVGL_DOUBLE_DISP_DEMO) || defined(LV_USE_SPI_REPLACE_MIPI_DBI) || defined(AIC_LVGL_AI_EYES_DEMO)  || defined(AIC_LVGL_FIVE_DISP_DEMO)
    int use_frame_buffer = 0;
#if defined(LV_USE_SPI_REPLACE_MIPI_DBI) && !defined(AIC_LVGL_FIVE_DISP_DEMO)
    use_frame_buffer = 1;
#endif
    extern int lv_spi_display_init(int use_frame_buffer);
    if (lv_spi_display_init(use_frame_buffer) < 0) {
        LV_LOG_ERROR("init spi display failed");
        return;
    }
#endif

#ifndef AIC_MONKEY_TEST
#if defined(KERNEL_RTTHREAD) && defined(AIC_USING_TOUCH)
    int result = tpc_run(AIC_TOUCH_PANEL_NAME, fb_info.width, fb_info.height);
    if (result) {
        LV_LOG_INFO("can't find touch panel\n");
    }
#endif
#endif
}

#endif // LPKG_MPP
