/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <getopt.h>

#include <drivers/rtc.h>
#include <drivers/alarm.h>

#include <aic_drv_irq.h>

#include "aic_core.h"
#include "hal_rtc.h"

#define AIC_RTC_NAME            "aic-rtc"

#define AIC_RTC_EPOCH           1589932800ULL /* 2020-05-20 00:00:00 */
#define RTC_MAX_SEC             0xFFFFFFFFULL
#define SEC_PER_YEAR            (365 * 24 * 3600)

#undef pr_debug
#ifdef AIC_RTC_DRV_DEBUG
#define pr_debug    pr_info
#else
#define pr_debug(fmt, ...)
#endif

static rt_rtc_dev_t aic_rtc;

static rt_err_t rtc_ops_init(void)
{
    hal_rtc_init();
#ifdef RT_USING_ALARM
    aicos_request_irq(RTC_IRQn, hal_rtc_irq, 0, NULL, NULL);
#endif
    hal_rtc_cali(AIC_RTC_CLK_RATE);

#if defined(AIC_RTC_ALARM_IO_OUTPUT)
    hal_rtc_alarm_io_output();
#elif defined(AIC_RTC_32K_IO_OUTPUT)
    hal_rtc_32k_clk_output();
#endif

    return RT_EOK;
}

static rt_err_t rtc_ops_get_secs(time_t *sec)
{
    struct tm tm = {0};
    u32 rtc_sec = 0;

    hal_rtc_read_time(&rtc_sec);
    *sec = rtc_sec + AIC_RTC_EPOCH;

    /* Only for debug log */
    gmtime_r(sec, &tm);
    pr_debug("Get RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    return RT_EOK;
}

static rt_err_t rtc_ops_set_secs(time_t *sec)
{
    s32 remain_year = 0;
    struct tm tm = {0};
    u32 rtc_sec = 0;

    if (*sec < AIC_RTC_EPOCH || *sec > (time_t)(RTC_MAX_SEC + AIC_RTC_EPOCH)) {
        pr_err("RTC time is out of range: %ld\n", *sec);
        return -RT_EINVAL;
    }

    rtc_sec = (u32)(*sec - AIC_RTC_EPOCH);
    remain_year = (RTC_MAX_SEC - rtc_sec) / SEC_PER_YEAR;
    if (remain_year < 5)
        pr_warn("RTC approaching the time limit! Remaining %d years\n", remain_year);

    hal_rtc_set_time(rtc_sec);

    /* Only for debug log */
    gmtime_r(sec, &tm);
    pr_debug("Set RTC time: %04d-%02d-%02d %02d:%02d:%02d (remain %d years)\n",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec, remain_year);

#ifdef RT_USING_ALARM
    rt_alarm_update(&aic_rtc.parent, 1);
#endif
    return RT_EOK;
}

#ifdef RT_USING_ALARM
static int rtc_alarm_event(void)
{
    pr_debug("alarm interrupt\n");
#ifdef RT_USING_ALARM
    rt_alarm_update(NULL, 1);
#endif
    return 0;
}
#endif

static rt_err_t rtc_ops_get_alarm(struct rt_rtc_wkalarm *alarm)
{
#ifdef RT_USING_ALARM
    time_t alarm_sec = 0;
    struct tm tm = {0};

    alarm->enable = hal_rtc_read_alarm((u32 *)&alarm_sec);
    alarm_sec += AIC_RTC_EPOCH;
    gmtime_r(&alarm_sec, &tm);

    alarm->tm_sec  = tm.tm_sec;
    alarm->tm_min  = tm.tm_min;
    alarm->tm_hour = tm.tm_hour;
    pr_debug("Get alarm time: %04d-%02d-%02d %02d:%02d:%02d\n",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    return RT_EOK;
#else
    return -RT_ERROR;
#endif
}

static rt_err_t rtc_ops_set_alarm(struct rt_rtc_wkalarm *alarm)
{
#ifdef RT_USING_ALARM
    time_t cur_sec = 0;
    struct tm tm = {0};

    if (!alarm->enable) {
        pr_debug("Need not enable alarm\n");
        return RT_EOK;
    }

    rtc_ops_get_secs(&cur_sec);
    gmtime_r(&cur_sec, &tm);

    /* if the alarm will timeout in the next day */
    if (alarm->tm_hour < tm.tm_hour) {
        cur_sec += 3600 * 24;
        gmtime_r(&cur_sec, &tm);
    }

    tm.tm_hour = alarm->tm_hour;
    tm.tm_min  = alarm->tm_min;
    tm.tm_sec  = alarm->tm_sec;

    hal_rtc_register_callback(rtc_alarm_event);
    hal_rtc_set_alarm((u32)(timegm(&tm) - AIC_RTC_EPOCH));
    pr_debug("Set a alarm(%d): %04d-%02d-%02d %02d:%02d:%02d\n",
             alarm->enable, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    return RT_EOK;
#else
    return -RT_ERROR;
#endif
}

static const struct rt_rtc_ops aic_rtc_ops =
{
    rtc_ops_init,
    rtc_ops_get_secs,
    rtc_ops_set_secs,
    rtc_ops_get_alarm,
    rtc_ops_set_alarm,
    RT_NULL,
    RT_NULL,
};

int drv_rtc_init(void)
{
    rt_err_t ret = RT_EOK;

    aic_rtc.ops = &aic_rtc_ops;
    ret = rt_hw_rtc_register(&aic_rtc, "rtc", RT_DEVICE_FLAG_RDWR,
                             RT_NULL);
    if (ret != RT_EOK) {
        pr_err("Failed to register RTC: %d\n", ret);
        return ret;
    }
    pr_info("ArtInChip RTC loaded\n");

    return RT_EOK;
}
INIT_BOARD_EXPORT(drv_rtc_init);

