/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_TIME_H__
#define __AIC_TIME_H__

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NS_PER_SEC      1000000000
#define US_PER_SEC      1000000
#define MS_PER_SEC      1000

void aic_udelay(u32 us);
void aic_mdelay(u32 ms);
u64 aic_get_ticks(void);
u64 aic_get_time_us(void);
u64 aic_get_time_ms(void);

void gettimespec(struct timespec *t);
float timespec_diff(struct timespec *start, struct timespec *end);
void show_timespec_diff(char *head, char *tail,
                        struct timespec *start, struct timespec *end);
void show_fps(char *mod, struct timespec *start, struct timespec *end, int cnt);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_TIME_H__ */
