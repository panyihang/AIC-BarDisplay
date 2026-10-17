/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _MPP_FB_H_
#define _MPP_FB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>
#include <artinchip_fb.h>

struct mpp_fb;

struct mpp_fb *mpp_fb_open(void);

void mpp_fb_close(struct mpp_fb *fb);

int mpp_fb_probe(void);

void mpp_fb_remove(void);

int mpp_fb_ioctl(struct mpp_fb *fb, int cmd, void *args);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MPP_FB_H_ */
