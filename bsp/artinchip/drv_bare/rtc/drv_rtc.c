/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Weihui.Xu <weihui.xu@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>

#include "hal_rtc.h"
#include "drv_rtc.h"
#include "aic_soc.h"

/* timezone */
#ifndef RT_LIBC_DEFAULT_TIMEZONE
#define RT_LIBC_DEFAULT_TIMEZONE    8
#endif

static const char *days = "Sun Mon Tue Wed Thu Fri Sat ";
static const char *months = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";

static volatile char _current_timezone = RT_LIBC_DEFAULT_TIMEZONE;

void tz_set(char tz)
{
    _current_timezone = tz;
}

char tz_get(void)
{
    return _current_timezone;
}

static void num2str(char *c, int i)
{
    c[0] = i / 10 + '0';
    c[1] = i % 10 + '0';
}

static int __isleap(int year)
{
    /* every fourth year is a leap year except for century years that are
     * not divisible by 400. */
    /*  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)); */
    return (!(year % 4) && ((year % 100) || !(year % 400)));
}

time_t timegm(struct tm * const t)
{
    time_t day;
    time_t i;
    time_t years;

    if(t == RTC_NULL)
        return (time_t)RTC_ERR;

    years = (time_t)t->tm_year - 70;
    if (t->tm_sec > 60) {        /* seconds after the minute - [0, 60] including leap second */
        t->tm_min += t->tm_sec / 60;
        t->tm_sec %= 60;
    }
    if (t->tm_min >= 60) {       /* minutes after the hour - [0, 59] */
        t->tm_hour += t->tm_min / 60;
        t->tm_min %= 60;
    }
    if (t->tm_hour >= 24) {     /* hours since midnight - [0, 23] */
        t->tm_mday += t->tm_hour / 24;
        t->tm_hour %= 24;
    }
    if (t->tm_mon >= 12) {      /* months since January - [0, 11] */
        t->tm_year += t->tm_mon / 12;
        t->tm_mon %= 12;
    }
    while (t->tm_mday > __spm[1 + t->tm_mon]) {
        if (t->tm_mon == 1 && __isleap(t->tm_year + 1900)) {
            --t->tm_mday;
        }

        t->tm_mday -= __spm[t->tm_mon];
        ++t->tm_mon;
        if (t->tm_mon > 11) {
            t->tm_mon = 0;
            ++t->tm_year;
        }
    }

    if (t->tm_year < 70)
        return (time_t) RTC_ERR;

    /* Days since 1970 is 365 * number of years + number of leap years since 1970 */
    day = years * 365 + (years + 1) / 4;

    /* After 2100 we have to substract 3 leap years for every 400 years
     This is not intuitive. Most mktime implementations do not support
     dates after 2059, anyway, so we might leave this out for it's
     bloat. */
    if (years >= 131) {
        years -= 131;
        years /= 100;
        day -= (years >> 2) * 3 + 1;
        if ((years &= 3) == 3)
            years--;
        day -= years;
    }

    day += t->tm_yday = __spm[t->tm_mon] + t->tm_mday - 1 +
                        (__isleap(t->tm_year + 1900) & (t->tm_mon > 1));

    /* day is now the number of days since 'Jan 1 1970' */
    i = 7;
    t->tm_wday = (day + 4) % i; /* Sunday=0, Monday=1, ..., Saturday=6 */

    i = 24;
    day *= i;
    i = 60;
    return ((day + t->tm_hour) * i + t->tm_min) * i + t->tm_sec;
}

struct tm *gmtime_bare(const time_t *timep, struct tm *r)
{
    time_t i;
    time_t work = *timep % (SPD);

    if(timep == RTC_NULL || r == RTC_NULL)
        return RTC_NULL;

    memset(r, RTC_NULL, sizeof(struct tm));

    r->tm_sec = work % 60;
    work /= 60;
    r->tm_min = work % 60;
    r->tm_hour = work / 60;
    work = *timep / (SPD);
    r->tm_wday = (4 + work) % 7;
    for (i = 1970;; ++i) {
        time_t k = __isleap(i) ? 366 : 365;
        if (work >= k)
            work -= k;
        else
            break;
    }
    r->tm_year = i - 1900;
    r->tm_yday = work;

    r->tm_mday = 1;
    if (__isleap(i) && (work > 58)) {
        if (work == 59)
            r->tm_mday = 2; /* 29.2. */
        work -= 1;
    }

    for (i = 11; i && (__spm[i] > work); --i){};

    r->tm_mon = i;
    r->tm_mday += work - __spm[i];
    return r;
}

int rtc_get_secs(time_t *sec)
{
    struct tm tm = {0};

    hal_rtc_read_time((u32 *)sec);

    /* Only for debug log */
    gmtime_bare((time_t *)sec, &tm);
    pr_debug("Get RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
        tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    return RTC_OK;
}

int rtc_set_secs(time_t *sec)
{
    struct tm tm = {0};

    gmtime_bare((time_t *)sec, &tm);
    if (tm.tm_year < 100)
        return RTC_ERR;

    hal_rtc_set_time(*(time_t *)sec);
    pr_debug("Set RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
        tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);

    return RTC_OK;
}

#ifdef RT_USING_ALARM
static int rtc_alarm_event(void)
{
    pr_debug("alarm interrupt\n");
    return RTC_OK;
}
#endif

int rtc_get_alarm(struct rtc_wkalarm *alarm)
{
#ifdef RT_USING_ALARM
    time_t alarm_sec = 0;
    struct tm tm = {0};

    alarm->enable = hal_rtc_read_alarm((u32 *)&alarm_sec);
    gmtime_bare(&alarm_sec, &tm);

    alarm->tm_sec  = tm.tm_sec;
    alarm->tm_min  = tm.tm_min;
    alarm->tm_hour = tm.tm_hour;
    pr_debug("Get alarm time: %04d-%02d-%02d %02d:%02d:%02d\n",
        tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    return RTC_OK;
#else
    return RTC_ERR;
#endif
}

int rtc_set_alarm(struct rtc_wkalarm *alarm)
{
#ifdef RT_USING_ALARM
    time_t cur_sec = 0;
    struct tm tm = {0};

    if (!alarm->enable) {
        pr_debug("Need not enable alarm\n");
        return RTC_OK;
    }

    rtc_get_secs(&cur_sec);
    gmtime_bare(&cur_sec, &tm);

    /* if the alarm will timeout in the next day */
    if (alarm->tm_hour < tm.tm_hour) {
        cur_sec += 3600 * 24;
        gmtime_bare(&cur_sec, &tm);
    }

    tm.tm_hour = alarm->tm_hour;
    tm.tm_min  = alarm->tm_min;
    tm.tm_sec  = alarm->tm_sec;

    hal_rtc_register_callback(rtc_alarm_event);
    hal_rtc_set_alarm((u32)timegm(&tm));
    pr_debug("Set a alarm(%d): %04d-%02d-%02d %02d:%02d:%02d\n",
             alarm->enable, tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    return RTC_OK;
#else
    return RTC_ERR;
#endif
}

static char *time_format(struct tm *t, char *buf)
{
    if(t == RTC_NULL || buf == RTC_NULL)
        return RTC_NULL;

    memset(buf, RTC_NULL, 26);

    /* Checking input validity */
    if ((int)strlen(days) <= (t->tm_wday << 2) || (int)strlen(months) <= (t->tm_mon << 2)) {
        pr_debug("time_format: the input parameters exceeded the limit, please check it.");
        *(int*) buf = *(int*) days;
        *(int*) (buf + 4) = *(int*) months;
        num2str(buf + 8, t->tm_mday);
        if (buf[8] == '0')
            buf[8] = ' ';
        buf[10] = ' ';
        num2str(buf + 11, t->tm_hour);
        buf[13] = ':';
        num2str(buf + 14, t->tm_min);
        buf[16] = ':';
        num2str(buf + 17, t->tm_sec);
        buf[19] = ' ';
        num2str(buf + 20, 2000 / 100);
        num2str(buf + 22, 2000 % 100);
        buf[24] = '\n';
        buf[25] = '\0';


        return buf;
    }

    /* "Wed Jun 30 21:49:08 1993\n" */
    *(int*) buf = *(int*) (days + (t->tm_wday << 2));
    *(int*) (buf + 4) = *(int*) (months + (t->tm_mon << 2));
    num2str(buf + 8, t->tm_mday);
    if (buf[8] == '0')
        buf[8] = ' ';
    buf[10] = ' ';
    num2str(buf + 11, t->tm_hour);
    buf[13] = ':';
    num2str(buf + 14, t->tm_min);
    buf[16] = ':';
    num2str(buf + 17, t->tm_sec);
    buf[19] = ' ';
    num2str(buf + 20, (t->tm_year + 1900) / 100);
    num2str(buf + 22, (t->tm_year + 1900) % 100);
    buf[24] = '\n';
    buf[25] = '\0';
    return buf;
}

struct tm *get_localtime(void)
{
    static struct tm *r = RTC_NULL;
    u32 sec;
    time_t local_tz;

    r = malloc(sizeof(struct tm));
    if (!r) {
        pr_debug("get_localtime get mem fail");
        return 0;
    }
    memset(r, RTC_NULL, sizeof(struct tm));

    hal_rtc_read_time(&sec);
    local_tz = sec;

    return gmtime_bare(&local_tz, r);
}

char *local_time(void)
{
    static char buf[26];

    return time_format(get_localtime(), buf);
}

int drv_rtc_init(void)
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

    return RTC_OK;
}
