/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "mpp_fb.h"

#ifndef DISP_SHOW_FPS
#define DISP_SHOW_FPS 1
#endif

#ifndef LV_DISP_FB_DUMP
#define LV_DISP_FB_DUMP 0
#endif

typedef struct {
    struct mpp_fb *fb;
    struct aicfb_screeninfo info;
#if LV_USE_OS && defined(AIC_PAN_DISPLAY)
    lv_thread_t thread;
    lv_thread_sync_t sync;
    lv_thread_sync_t sync_notify;
    bool exit_status;
    bool sync_ready;
    bool flush_act;
#endif
    uint8_t *buf; /* draw buffer */
    uint8_t *buf_aligned;
    int  buf_size;
    int fb_size;
    int buf_id;
    int fps;
} aic_disp_t;

void lv_port_disp_init(void);

static inline int fbdev_draw_fps()
{
#if DISP_SHOW_FPS
    lv_display_t *disp = lv_display_get_default();
    aic_disp_t *aic_disp = (aic_disp_t *)lv_display_get_user_data(disp);
    return aic_disp->fps;
#else
    return 0;
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_PORT_DISP_H*/
