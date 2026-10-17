/*
* Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
*
*  SPDX-License-Identifier: Apache-2.0
*  author: <qi.xu@artinchip.com>
*  Desc:  packet (video bitstream container) manager
*
*/

#define LOG_TAG "packet_manager"

#include <string.h>
//#include <pthread.h>
#include "mpp_dec_type.h"
#include "mpp_mem.h"
#include "ve_buffer.h"
#include "packet_manager.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "aic_core.h"
#include "packet_allocator.h"

struct packet_impl {
    struct packet pkt;

    size_t pos_offset;
    struct mpp_list list;
};

struct packet_manager {
    int limit_count;
    int packet_count;
    struct packet_allocator *allocator;
    //pthread_mutex_t lock;
    aicos_mutex_t lock;
    struct mpp_list empty_list;
    struct mpp_list ready_list;
    struct mpp_list filling_list;
    int empty_num;
    int ready_num;
    struct packet_impl *packet_node;

    struct ve_buffer *mpp_buf;
    struct ve_buffer_allocator *ve_buf_handle;

    unsigned int buffer_phy;        // phy start addr of stream buffer
    unsigned char *buffer_start;        // virtual start addr of stream buffer
    unsigned char *buffer_end;        // virtual end addr of stream buffer
    int buffer_size;            // total size of stream buffer

    int write_offset;
    int read_offset;
    int available_size;
};

struct packet_manager *pm_create(struct packet_manager_init_cfg *cfg)
{
    struct packet_manager_init_cfg *init_cfg = cfg;
    struct packet_manager *pm;
    struct packet_impl *pkt_impl;
    int i;

    logd("create packet manager");

    if (!init_cfg || !init_cfg->ve_buf_handle || init_cfg->packet_count <= 0 || init_cfg->buffer_size <= 0)
        return NULL;

    pm = (struct packet_manager *)mpp_alloc(sizeof(struct packet_manager));
    if (!pm)
        return NULL;
    memset(pm, 0, sizeof(*pm));

    pm->empty_num = init_cfg->packet_count;
    pm->ready_num = 0;
    pm->packet_count = init_cfg->packet_count;
    pm->packet_node = (struct packet_impl *)mpp_alloc(pm->packet_count * sizeof(struct packet_impl));
    if (!pm->packet_node) {
        loge("alloc %d count packet failed!", pm->packet_count);
        mpp_free(pm);
        return NULL;
    }
    memset(pm->packet_node, 0, pm->packet_count * sizeof(struct packet_impl));

    pm->ve_buf_handle = init_cfg->ve_buf_handle;
    pm->buffer_size = init_cfg->buffer_size;
    pm->allocator = init_cfg->pkt_allocator;
    if (pm->allocator) {
        if (pm->allocator->ops && pm->allocator->ops->allocator_init) {
            pm->allocator->ops->allocator_init(pm->allocator);
        }
    } else {
        pm->mpp_buf = ve_buffer_alloc(pm->ve_buf_handle, pm->buffer_size, ALLOC_NEED_VIR_ADDR);
        if (!pm->mpp_buf) {
            loge("alloc mpp buffer %d failed!", pm->buffer_size);
            mpp_free(pm->packet_node);
            mpp_free(pm);
            return NULL;
        }

        logd("packet manager create %d count packet, buffer size %d", pm->packet_count, pm->buffer_size);

        pm->buffer_start = pm->mpp_buf->vir_addr;
        pm->buffer_end = pm->mpp_buf->vir_addr + pm->mpp_buf->size - 1;
        pm->buffer_size = pm->mpp_buf->size;
        pm->buffer_phy = pm->mpp_buf->phy_addr;

        pm->read_offset = 0;
        pm->write_offset = 0;
        pm->available_size = pm->mpp_buf->size;
    }
    //pthread_mutex_init(&pm->lock, NULL);
    pm->lock = aicos_mutex_create();


    mpp_list_init(&pm->empty_list);
    mpp_list_init(&pm->ready_list);

    mpp_list_init(&pm->filling_list);

    pkt_impl = pm->packet_node;
    for (i = 0; i < pm->packet_count; i++) {
        mpp_list_init(&pkt_impl->list);
        mpp_list_add_tail(&pkt_impl->list, &pm->empty_list);
        pkt_impl++;
    }

    logd("create packet manager successful! (%p)", pm);

    return pm;
}

int pm_destroy(struct packet_manager *pm)
{
    logd("destroy packet manager");

    if (!pm)
        return -1;

    //pthread_mutex_destroy(&pm->lock);

    aicos_mutex_delete(pm->lock);

    /* External allocators own their buffers even without a deinit callback. */
    if (pm->allocator) {
        if (pm->allocator->ops && pm->allocator->ops->allocator_deinit)
            pm->allocator->ops->allocator_deinit(pm->allocator);
    } else if (pm->mpp_buf) {
        ve_buffer_free(pm->ve_buf_handle, pm->mpp_buf);
    }

    if (pm->packet_node)
        mpp_free(pm->packet_node);

    if (pm)
        mpp_free(pm);

    return 0;
}

int pm_dequeue_empty_packet(struct packet_manager *pm, struct mpp_packet *packet, int size)
{
    struct packet_impl *pkt_impl;
    int left_size;        // remain size in end of stream buffer
    //int write_offset = 0;

    if (!pm || !packet)
        return -1;

    if (!pm->allocator && size <= 0) {
        return -1;
    }

    if (pm->allocator) {
        int ret = 0;

        aicos_mutex_take(pm->lock,AICOS_WAIT_FOREVER);
        pkt_impl = mpp_list_first_entry_or_null(&pm->empty_list, struct packet_impl, list);
        if (!pkt_impl) {
            logw("packet manager dequeue mpp packet failed!");
            aicos_mutex_give(pm->lock);
            return -1;
        }
        ret = pm->allocator->ops->alloc(pm->allocator,packet);
        if (ret) {
            logw("packet manager dequeue mpp packet failed!");
            aicos_mutex_give(pm->lock);
            return -1;
        }

        pkt_impl->pkt.data = packet->data; // data  phy_base +  phy_offset;
        pkt_impl->pkt.phy_base =(unsigned long)packet->data;
        pkt_impl->pkt.phy_size = packet->size;
        pkt_impl->pkt.size = packet->size;
        pkt_impl->pkt.phy_offset = 0; // offset int
        pkt_impl->pkt.flag = packet->flag;

        mpp_list_del_init(&pkt_impl->list);
        mpp_list_add_tail(&pkt_impl->list, &pm->filling_list);

        pm->empty_num --;
        aicos_mutex_give(pm->lock);
        return 0;
    }

    logd("packet manager dequeue empty mpp packet, pm: %p, packet: %p, size: %d", pm, packet, size);

    //pthread_mutex_lock(&pm->lock);
    aicos_mutex_take(pm->lock,AICOS_WAIT_FOREVER);
    left_size = pm->buffer_size - pm->write_offset;

    if (pm->available_size < size) {
        logw("packet manager dequeue mpp packet size %d > available size %d", size, pm->available_size);
        //pthread_mutex_unlock(&pm->lock);
        aicos_mutex_give(pm->lock);
        return -1;
    }

    if (pm->write_offset >= pm->read_offset) {
        if (left_size < size && pm->read_offset < size) {
            logw("packet manager dequeue mpp packet size(%d) failed, read offset %d write offset %d", size,
                 pm->read_offset, pm->write_offset);
            //pthread_mutex_unlock(&pm->lock);
            aicos_mutex_give(pm->lock);
            return -1;
        }
    }

    pkt_impl = mpp_list_first_entry_or_null(&pm->empty_list, struct packet_impl, list);
    if(!pkt_impl) {
        logw("packet manager dequeue mpp packet failed!");
        //pthread_mutex_unlock(&pm->lock);
        aicos_mutex_give(pm->lock);
        return -1;
    }

    pkt_impl->pkt.size = size;
    pkt_impl->pkt.phy_offset = pm->write_offset;
    pkt_impl->pkt.phy_size = pm->buffer_size;
    pkt_impl->pkt.phy_base = pm->buffer_phy;
    pkt_impl->pkt.data = pm->buffer_start + pm->write_offset;
    pkt_impl->pos_offset = 0;

    if (pm->write_offset >= pm->read_offset) {
        if (left_size >= size) {
            // if left_size is enough to store this packet
            pm->write_offset += size;
            pm->available_size -= size;

            if (pm->write_offset == pm->buffer_size)
                pm->write_offset = 0;
        } else if (pm->read_offset >= size) {
            logw("left_size: %d, size: %d", left_size, size);
            // if left_size is not enough to store this packet,
            //  we store this packet from the start of stream buffer
            pm->write_offset = size;
            pm->available_size -= (size + left_size);
            pkt_impl->pos_offset = left_size;
            pkt_impl->pkt.data = pm->buffer_start;
            pkt_impl->pkt.phy_offset = 0;
        } else {
            // left_size is not enough to store this packet,
            // and there is no buffer in start of stream buffer
            logw("packet manager dequeue mpp packet failed!");
            //pthread_mutex_unlock(&pm->lock);
            aicos_mutex_give(pm->lock);
            return -1;
        }
    } else {
        pm->write_offset += size;
        pm->available_size -= size;
    }

    logw("get empty packet phy_offset: %d, size: %d", pm->write_offset, size);

    packet->data = pkt_impl->pkt.data;
    packet->size = pkt_impl->pkt.size;
    pm->empty_num --;

    //pthread_mutex_unlock(&pm->lock);
    aicos_mutex_give(pm->lock);

    return 0;
}

int pm_enqueue_ready_packet(struct packet_manager *pm, struct mpp_packet *packet)
{
    struct packet_impl *pkt_impl;
    size_t left_offset;

    logd("packet manager enqueue ready mpp packet");

    if (!pm || !packet)
        return -1;

    //pthread_mutex_lock(&pm->lock);
    aicos_mutex_take(pm->lock,AICOS_WAIT_FOREVER);
    if(pm->allocator) {
        pkt_impl = mpp_list_first_entry_or_null(&pm->filling_list, struct packet_impl, list);
    } else {
        pkt_impl = mpp_list_first_entry_or_null(&pm->empty_list, struct packet_impl, list);
    }

    if(!pkt_impl) {
        logw("packet manager enqueue ready mpp packet failed!");
        //pthread_mutex_unlock(&pm->lock);
        aicos_mutex_give(pm->lock);
        return -1;
    }

    if (pkt_impl->pkt.data != packet->data || pkt_impl->pkt.size < packet->size) {
        logw("packet manager enqueue ready mpp packet check failed!");
        //pthread_mutex_unlock(&pm->lock);
        aicos_mutex_give(pm->lock);
        return -1;
    }

    if(!pm->allocator) {
        if (pkt_impl->pkt.size > packet->size) {
            left_offset = pkt_impl->pkt.size - packet->size;
            if (pm->write_offset == 0) {
                pm->write_offset = pm->buffer_size - left_offset;
                pm->available_size += left_offset;
            } else {
                pm->write_offset -= left_offset;
                pm->available_size += left_offset;
            }
        }
    }

    pkt_impl->pkt.size = packet->size;
    pkt_impl->pkt.pts = packet->pts;
    pkt_impl->pkt.flag = packet->flag;
    //loge("pm_enqueue_ready_packet %lu\n", pkt_impl->pkt.size);
    mpp_list_del_init(&pkt_impl->list);
    mpp_list_add_tail(&pkt_impl->list, &pm->ready_list);
    pm->ready_num++;

    ve_buffer_sync_range(pm->mpp_buf, pkt_impl->pkt.data, pkt_impl->pkt.size, CACHE_CLEAN);

    //pthread_mutex_unlock(&pm->lock);
    aicos_mutex_give(pm->lock);

    return 0;
}

int pm_reclaim_ready_packet(struct packet_manager *pm, struct packet *packet)
{
    logd("packet manager return ready mpp packet");

    if (!pm || !packet)
        return -1;

    //pthread_mutex_lock(&pm->lock);
    aicos_mutex_take(pm->lock,AICOS_WAIT_FOREVER);

    pm->ready_num++;

    //pthread_mutex_unlock(&pm->lock);
    aicos_mutex_give(pm->lock);

    return 0;
}

struct packet *pm_dequeue_ready_packet(struct packet_manager *pm)
{
    struct packet_impl *pkt_impl;

    logd("packet manager dequeue ready packet");

    if (!pm)
        return NULL;

    //pthread_mutex_lock(&pm->lock);
    aicos_mutex_take(pm->lock,AICOS_WAIT_FOREVER);

    pkt_impl = mpp_list_first_entry_or_null(&pm->ready_list, struct packet_impl, list);
    if(!pkt_impl) {
        //pthread_mutex_unlock(&pm->lock);
        aicos_mutex_give(pm->lock);
        return NULL;
    }
    pm->ready_num--;

    //pthread_mutex_unlock(&pm->lock);
    aicos_mutex_give(pm->lock);

    return (struct packet *)pkt_impl;
}

int pm_enqueue_empty_packet(struct packet_manager *pm, struct packet *packet)
{
    struct packet_impl *pkt_impl = (struct packet_impl *)packet;
    size_t read_offset;

    logd("packet manager enqueue empty packet");

    if (!pm || !pkt_impl)
        return -1;

    //pthread_mutex_lock(&pm->lock);
    aicos_mutex_take(pm->lock,AICOS_WAIT_FOREVER);

   if (pm->allocator) {
        struct mpp_packet pkt;
        pkt.data = packet->data;
        pkt.flag = packet->flag;
        pkt.pts = packet->pts;
        pm->allocator->ops->free(pm->allocator,&pkt);
   }
    mpp_list_del_init(&pkt_impl->list);
    mpp_list_add_tail(&pkt_impl->list, &pm->empty_list);

    if (!pm->allocator) {
        read_offset = pm->read_offset + pkt_impl->pos_offset + pkt_impl->pkt.size;
        if (read_offset >= pm->buffer_size)
            read_offset -= pm->buffer_size;

        pm->read_offset = read_offset;
        if (pm->read_offset == pm->write_offset) {
            pm->read_offset = 0;
            pm->write_offset = 0;
            pm->available_size = pm->buffer_size;
        } else {
            pm->available_size += pkt_impl->pos_offset + pkt_impl->pkt.size;
        }
    }

    pm->empty_num++;

    //pthread_mutex_unlock(&pm->lock);
    aicos_mutex_give(pm->lock);

    return 0;
}

int pm_get_empty_packet_num(struct packet_manager *pm)
{
    if (!pm)
        return -1;
    return pm->empty_num;
}

int pm_get_ready_packet_num(struct packet_manager *pm)
{
    if (!pm)
        return -1;
    return pm->ready_num;
}

int pm_reset(struct packet_manager *pm)
{
    if (!pm)
        return -1;

    aicos_mutex_take(pm->lock,AICOS_WAIT_FOREVER);
    if (!mpp_list_empty(&pm->ready_list)) {
        struct packet_impl *pkt1=NULL,*pkt2=NULL;
        mpp_list_for_each_entry_safe(pkt1,pkt2,&pm->ready_list,list) {
            mpp_list_del_init(&pkt1->list);
            mpp_list_add_tail(&pkt1->list, &pm->empty_list);
        }
    }

    logd("read_offset:%d,write_offset:%d,empty_num:%d,ready_num:%d,available_size:%d\n"
    ,pm->read_offset
    ,pm->write_offset
    ,pm->empty_num
    ,pm->ready_num
    ,pm->available_size);

    pm->empty_num = pm->packet_count;
    pm->ready_num = 0;
    pm->read_offset = 0;
    pm->write_offset = 0;
    pm->available_size = pm->buffer_size;
    aicos_mutex_give(pm->lock);
    return 0;
}
