/*
* Copyright (C) 2020-2022 Artinchip Technology Co. Ltd
*
*  author: <qi.xu@artinchip.com>
*  Desc: allocator of physic continuous buffer used by ve
*/

#define LOG_TAG "ve_buffer"
#include "aic_core.h"
#include "ve_buffer.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "dma_allocator.h"
#include "ve.h"
#include "mpp_log.h"

#define align_cache_line(addr, len, a, l) \
unsigned long a = (unsigned long)addr; \
unsigned long l = (unsigned long)len; \
if (addr % CACHE_LINE_SIZE) { \
    a -= (addr %  CACHE_LINE_SIZE); \
    l += (addr %  CACHE_LINE_SIZE); \
} \
if (l % CACHE_LINE_SIZE) { \
    l += CACHE_LINE_SIZE - (l % CACHE_LINE_SIZE); \
}

struct ve_buffer_allocator {
    unsigned int addr;
    int total_count;
    int total_size;
    aicos_mutex_t lock;
    struct mpp_list list;
};

struct ve_buffer_impl {
    struct ve_buffer ve_buf;
    struct mpp_list list_status;
};

struct ve_buffer_allocator *ve_buffer_allocator_create(enum ve_buffer_type type)
{
    struct ve_buffer_allocator *ctx;

    ctx = mpp_alloc(sizeof(struct ve_buffer_allocator));
    if (!ctx)
        return NULL;
    memset(ctx, 0, sizeof(struct ve_buffer_allocator));

    ctx->total_size = 0;
    ctx->total_count = 0;

    ctx->lock = aicos_mutex_create();
    mpp_list_init(&ctx->list);

    return ctx;
}

void ve_buffer_allocator_destroy(struct ve_buffer_allocator *ctx)
{
    struct ve_buffer_impl *node = NULL;
    struct ve_buffer_impl *m = NULL;

    if (!ctx)
        return;

    aicos_mutex_take(ctx->lock,AICOS_WAIT_FOREVER);

    if (!mpp_list_empty(&ctx->list)) {
        mpp_list_for_each_entry_safe(node, m, &ctx->list, list_status) {
            ctx->total_size -= node->ve_buf.size;
            ctx->total_count--;
            mpp_list_del_init(&node->list_status);

            logw("ve_buffer leak, dma-buf fd: %d, size: %d", node->ve_buf.fd, node->ve_buf.size);
            mpp_phy_free((unsigned long)node->ve_buf.vir_addr);

            mpp_free(node);
            node = NULL;
        }
    }
    mpp_list_del_init(&ctx->list);

    aicos_mutex_give(ctx->lock);

    if (ctx->total_count > 0) {
        loge("mppbuffer ctx deinit, there are %d buffers not released.", ctx->total_count);
    }

    aicos_mutex_delete(ctx->lock);

    mpp_free(ctx);
}

struct ve_buffer *ve_buffer_alloc(struct ve_buffer_allocator *ctx, int size, enum ve_buffer_flag flag)
{
    logd("ve_buffer_alloc, size: %d", size);
    struct ve_buffer_impl *buf_impl;

    if (!ctx || size <= 0) {
        return NULL;
    }

    buf_impl = (struct ve_buffer_impl*)mpp_alloc(sizeof(struct ve_buffer_impl));
    if(!buf_impl) {
        return NULL;
    }
    memset(buf_impl, 0, sizeof(struct ve_buffer_impl));

    // 1. alloc dma-buf
    buf_impl->ve_buf.phy_addr = mpp_phy_alloc(size);
    buf_impl->ve_buf.vir_addr = (unsigned char*)buf_impl->ve_buf.phy_addr;
    buf_impl->ve_buf.size = size;
    if (buf_impl->ve_buf.phy_addr == 0) {
        free(buf_impl);
        return NULL;
    }

    aicos_mutex_take(ctx->lock,AICOS_WAIT_FOREVER);

    ctx->total_size += size;
    ctx->total_count++;

    mpp_list_init(&buf_impl->list_status);
    mpp_list_add_tail(&buf_impl->list_status, &ctx->list);

    aicos_mutex_give(ctx->lock);

    logi("ve_buffer alloc success, fd: %d, size: %d", buf_impl->ve_buf.fd, buf_impl->ve_buf.size);

    return (struct ve_buffer *)buf_impl;
}

int ve_buffer_sync(struct ve_buffer *buf, enum dma_buf_sync_flag flag)
{
    unsigned long* addr = (unsigned long*)buf->vir_addr;
    int size = buf->size;

    if(flag == CACHE_CLEAN) {
        align_cache_line((unsigned long)addr, size, a, l);
        aicos_dcache_clean_range((unsigned long *)a, (int64_t)l);
    } else if (flag == CACHE_INVALID) {
        align_cache_line((unsigned long)addr, size, a, l);
        aicos_dcache_invalid_range((unsigned long *)a, (int64_t)l);
    } else {
        logw("unkown cache flag: %d", flag);
        return -1;
    }

    return 0;
}

int ve_buffer_sync_range(struct ve_buffer *buf, unsigned char* start_addr, int size, enum dma_buf_sync_flag flag)
{
    if(flag == CACHE_CLEAN) {
        align_cache_line((unsigned long)start_addr, size, a, l);
        aicos_dcache_clean_range((unsigned long *)a, (int64_t)l);
    } else if (flag == CACHE_INVALID) {
        align_cache_line((unsigned long)start_addr, size, a, l);
        aicos_dcache_invalid_range((unsigned long *)a, (int64_t)l);
    } else {
        logw("unkown cache flag: %d", flag);
        return -1;
    }

    return 0;
}

void ve_buffer_free(struct ve_buffer_allocator *ctx, struct ve_buffer *buf)
{
    struct ve_buffer_impl* buf_impl = (struct ve_buffer_impl*)buf;

    if(!ctx || !buf)
        return;

    logi("ve_buffer_free, fd: %d, size: %d", buf->fd, buf->size);

    aicos_mutex_take(ctx->lock,AICOS_WAIT_FOREVER);

    ctx->total_size -= buf->size;
    ctx->total_count--;
    mpp_list_del_init(&buf_impl->list_status);

    aicos_mutex_give(ctx->lock);

    mpp_phy_free(buf->phy_addr);
    mpp_free(buf_impl);
}
