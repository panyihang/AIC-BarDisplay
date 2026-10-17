/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef LV_TPC_RUN
#define LV_TPC_RUN

#ifdef __cplusplus
extern "C" {
#endif

#include <rtconfig.h>

#ifdef KERNEL_RTTHREAD
#include <rtthread.h>
#include <rtdevice.h>

#ifdef AIC_USING_RTP
#define AIC_RTP_RECALIBRATE_ENABLE 0
#endif

enum {
    RTP_CAL_POINT_DIR_TOP_LEFT = 0,
    RTP_CAL_POINT_DIR_TOP_RIGHT,
    RTP_CAL_POINT_DIR_BOT_RIGHT,
    RTP_CAL_POINT_DIR_BOT_LEFT,
    RTP_CAL_POINT_DIR_CENTER,
};
typedef int rtp_cal_point_dir_t;

int tpc_run(const char *name, rt_uint16_t x, rt_uint16_t y);
#ifdef AIC_USING_TOUCH
void lvgl_get_tp(void);
void lvgl_put_tp(void);
#else
static void inline lvgl_get_tp(void)
{
}
static void inline lvgl_put_tp(void)
{
}
#endif
void rtp_check_event_type(int event_type, int pressure);

void rtp_init_recalibrate(void);
void rtp_get_calibrate_point(rtp_cal_point_dir_t dir, int *x, int *y);
int rtp_get_recalibrate_status(void);
int rtp_get_cur_recalibrate_adc_value(int *x, int *y);

#ifdef AIC_USING_TOUCH
void rtp_store_recalibrate_data(rt_device_t rtp_dev, struct rt_touch_data *data);
#endif

void rtp_update_recalibrate(void);
int rtp_is_recalibrate_started(void);
int rtp_is_recalibrate_all_data_stored(void);
int rtp_is_recalibrate_dir_data_stored(rtp_cal_point_dir_t dir);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_TPC_RUN */
