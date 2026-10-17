/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Weihui.Xu <weihui.xu@artinchip.com>
 */
#ifndef _DRV_RTC_H_
#define _DRV_RTC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <aic_core.h>

#define RTC_OK      0
#define RTC_ERR     (-1)
#define RTC_NULL    0

/* days per month -- nonleap! */
static const short __spm[13] =
{
    0,
    (31),
    (31 + 28),
    (31 + 28 + 31),
    (31 + 28 + 31 + 30),
    (31 + 28 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31),
};

/* seconds per day */
#define SPD (24 * 60 * 60)

/* used for alarm function */
struct rtc_wkalarm
{
    bool enable;             /* 0 = alarm disabled, 1 = alarm enabled */
    int tm_sec;               /* alarm at tm_sec */
    int tm_min;               /* alarm at tm_min */
    int tm_hour;              /* alarm at tm_hour */
};

void tz_set(char tz);
char tz_get(void);
time_t timegm(struct tm * const t);
struct tm *gmtime_bare(const time_t *timep, struct tm *r);
int rtc_get_secs(time_t *sec);
int rtc_set_secs(time_t *sec);
int rtc_get_alarm(struct rtc_wkalarm *alarm);
int rtc_set_alarm(struct rtc_wkalarm *alarm);
struct tm* get_localtime(void);
char *local_time(void);
int drv_rtc_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_RTC_H_ */
