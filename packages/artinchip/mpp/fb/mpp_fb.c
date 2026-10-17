/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#if defined(KERNEL_RTTHREAD)
#include <rtdevice.h>
#include <rtdef.h>
#endif

#include "mpp_fb.h"

struct mpp_fb {
#if defined(KERNEL_RTTHREAD)
    rt_device_t dev;
#endif
    unsigned int ref_count;
};

static struct mpp_fb *g_mpp_fb = NULL;

struct mpp_fb *mpp_fb_open(void)
{
#if defined(KERNEL_RTTHREAD)
    rt_device_t dev;
#endif
    struct mpp_fb *fb;

    if (g_mpp_fb) {
        g_mpp_fb->ref_count++;
        return g_mpp_fb;
    }

    fb = aicos_malloc(0, sizeof(struct mpp_fb));
    if (!fb) {
        pr_err("alloc mpp fb failed\n");
        return NULL;
    }

#if defined(KERNEL_RTTHREAD)
    dev = rt_device_find("aicfb");
    if (!dev) {
        pr_err("Failed to find aicfb device\n");
        aicos_free(0, fb);
        return NULL;
    }
    fb->dev = dev;
#endif

    fb->ref_count = 1;
    g_mpp_fb = fb;

    return fb;
}

void mpp_fb_close(struct mpp_fb *fb)
{
    if (!fb)
        return;

    fb->ref_count--;

    if (fb->ref_count == 0) {
        g_mpp_fb = NULL;
        free(fb);
    }
}

int mpp_fb_probe(void)
{
    return aicfb_probe();
}

void mpp_fb_remove(void)
{
    aicfb_remove();
}

int mpp_fb_ioctl(struct mpp_fb *fb, int cmd, void *args)
{
    int ret = 0;
#if defined(KERNEL_RTTHREAD)
    rt_device_t dev;

    dev = fb->dev;

    ret = rt_device_control(dev, cmd, args);
#elif defined(KERNEL_BAREMETAL) || defined(KERNEL_FREERTOS)
    ret = aicfb_ioctl(cmd, args);
#endif
    return ret;
}

