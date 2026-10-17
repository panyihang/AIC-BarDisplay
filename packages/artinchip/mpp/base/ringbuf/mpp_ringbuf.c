/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: mpp thread interface
 */

#include "mpp_ringbuf.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include "rtconfig.h"

#if defined(KERNEL_RTTHREAD)
#include "aic_osal.h"
#include <rtdevice.h>
#include <rtthread.h>
struct mpp_ringbuf_handle {
    struct rt_ringbuffer ringbuf;
    unsigned char *buf_pool;
    int buf_len;
};
#endif

mpp_ringbuf_t mpp_ringbuffer_create(int length)
{
    struct mpp_ringbuf_handle *h_ringbuf = NULL;

#if defined(KERNEL_RTTHREAD)
    h_ringbuf = (struct mpp_ringbuf_handle *)mpp_alloc(sizeof(struct mpp_ringbuf_handle));
    if (!h_ringbuf)
        return NULL;
    memset(h_ringbuf, 0, sizeof(struct mpp_ringbuf_handle));
    h_ringbuf->buf_pool = aicos_malloc_align(MEM_DEFAULT, length, 64);
    if (!h_ringbuf->buf_pool) {
        loge("ERROR: allocating buffer pool(%d) failed\n", length);
        mpp_free(h_ringbuf);
        return NULL;
    }
    rt_ringbuffer_init(&h_ringbuf->ringbuf, h_ringbuf->buf_pool, length);
#else
    /*Not support now*/
#endif
    return (mpp_ringbuf_t)h_ringbuf;
}

void mpp_ringbuffer_destroy(mpp_ringbuf_t rb)
{
    if (rb) {
#if defined(KERNEL_RTTHREAD)
        struct mpp_ringbuf_handle *h_ringbuf = (struct mpp_ringbuf_handle *)rb;
        aicos_free_align(MEM_DEFAULT, h_ringbuf->buf_pool);
        mpp_free(rb);
#else
        /*Not support now*/
#endif
    }
}

void mpp_ringbuffer_reset(mpp_ringbuf_t rb)
{
    if (rb) {
#if defined(KERNEL_RTTHREAD)
        struct mpp_ringbuf_handle *h_ringbuf = (struct mpp_ringbuf_handle *)rb;
        rt_ringbuffer_reset(&h_ringbuf->ringbuf);
#else
        /*Not support now*/
#endif
    }
}

int mpp_ringbuffer_put(mpp_ringbuf_t rb, const unsigned char *ptr, int length)
{
    if (!rb) {
        return -1;
    }
#if defined(KERNEL_RTTHREAD)
    struct mpp_ringbuf_handle *h_ringbuf = (struct mpp_ringbuf_handle *)rb;
    return rt_ringbuffer_put(&h_ringbuf->ringbuf, ptr, length);
#else
    /*Not support now*/
    return -1;
#endif
}

int mpp_ringbuffer_get(mpp_ringbuf_t rb, unsigned char *ptr, int length)
{
    if (!rb) {
        return -1;
    }
#if defined(KERNEL_RTTHREAD)
    struct mpp_ringbuf_handle *h_ringbuf = (struct mpp_ringbuf_handle *)rb;
    return rt_ringbuffer_get(&h_ringbuf->ringbuf, ptr, length);
#else
    /*Not support now*/
    return -1;
#endif
}

int mpp_ringbuffer_data_len(mpp_ringbuf_t rb)
{
    if (!rb) {
        return -1;
    }
#if defined(KERNEL_RTTHREAD)
    struct mpp_ringbuf_handle *h_ringbuf = (struct mpp_ringbuf_handle *)rb;
    return rt_ringbuffer_data_len(&h_ringbuf->ringbuf);
#else
    /*Not support now*/
    return -1;
#endif
}

int mpp_ringbuffer_space_len(mpp_ringbuf_t rb)
{
    if (!rb) {
        return -1;
    }
#if defined(KERNEL_RTTHREAD)
    struct mpp_ringbuf_handle *h_ringbuf = (struct mpp_ringbuf_handle *)rb;
    return rt_ringbuffer_space_len(&h_ringbuf->ringbuf);
#else
    /*Not support now*/
    return -1;
#endif
}
