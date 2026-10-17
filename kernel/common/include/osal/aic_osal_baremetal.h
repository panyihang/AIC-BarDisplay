/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_BAREMETAL_H_
#define _ARTINCHIP_AIC_OSAL_BAREMETAL_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <string.h>
#include <csi_core.h>
#include <aic_errno.h>
#include <aic_tlsf.h>
#include <aic_time.h>

//--------------------------------------------------------------------+
// Mem API
//--------------------------------------------------------------------+

static inline void *aicos_malloc(unsigned int mem_type, size_t size)
{
    return aic_tlsf_malloc(mem_type, size);
}

static inline void aicos_free(unsigned int mem_type, void *mem)
{
    aic_tlsf_free(mem_type, mem);
}

static inline void *aicos_malloc_align(uint32_t mem_type, size_t size, size_t align)
{
    return aic_tlsf_malloc_align(mem_type, size, align);
}

static inline void aicos_free_align(uint32_t mem_type, void *mem)
{
    aic_tlsf_free_align(mem_type, mem);
}

static inline void *aicos_memdup(unsigned int mem_type, void *src, size_t size)
{
    void *p;

    p = aic_tlsf_malloc(mem_type, size);
    if (p)
        memcpy(p, src, size);
    return p;
}

static inline void *aicos_memdup_align(unsigned int mem_type, void *src, size_t size, size_t align)
{
    void *p;

    p = aic_tlsf_malloc_align(mem_type, size, align);
    if (p)
        memcpy(p, src, size);
    return p;
}

//--------------------------------------------------------------------+
// Timeout Define
//--------------------------------------------------------------------+

#define AICOS_WAIT_FOREVER 0xffffffffu

//--------------------------------------------------------------------+
// Thread API
//--------------------------------------------------------------------+

static inline aicos_thread_t aicos_thread_create(const char *name, uint32_t stack_size, uint32_t prio, aic_thread_entry_t entry, void *args) {return NULL;}
static inline void aicos_thread_delete(aicos_thread_t thread) {}
static inline void aicos_thread_suspend(aicos_thread_t thread) {}
static inline void aicos_thread_resume(aicos_thread_t thread) {}

//--------------------------------------------------------------------+
// Counting Semaphore API
//--------------------------------------------------------------------+

typedef struct
{
  volatile unsigned short count;
}osal_semaphore_def_t;

typedef osal_semaphore_def_t* osal_semaphore_t;
typedef osal_semaphore_def_t aicos_mutex_s;

static inline aicos_sem_t aicos_sem_create(uint32_t initial_count)
{
    osal_semaphore_t sem;

    sem = aicos_malloc(0, sizeof(osal_semaphore_def_t));
    if (NULL == sem)
        return NULL;

    sem->count = initial_count;

    return (aicos_sem_t)sem;
}

static inline void aicos_sem_delete(aicos_sem_t sem)
{
    aicos_free(0, sem);
}

static inline int aicos_sem_take(aicos_sem_t sem, uint32_t msec)
{
    osal_semaphore_t sem_hdl = (osal_semaphore_t)sem;
    u64 s_us = 0;
    u64 e_us = 0;

    if (sem_hdl->count > 0) {
        sem_hdl->count--;
        return 0;
    }

    if (msec == 0) {
        return -EAGAIN;
    } else if (msec == AICOS_WAIT_FOREVER) {
        while (sem_hdl->count == 0) { }
    } else {
        s_us = aic_get_time_us();
        while (sem_hdl->count == 0) {
            e_us = aic_get_time_us();
            if (((e_us-s_us) / 1000) >= msec)
                return -ETIME;
        }
    }

    if (sem_hdl->count > 0) {
        sem_hdl->count--;
        return 0;
    }

    return -EAGAIN;
}

static inline int aicos_sem_give(aicos_sem_t sem)
{
    osal_semaphore_t sem_hdl = (osal_semaphore_t)sem;

    sem_hdl->count++;

    return 0;
}

static inline int aicos_sem_reset(aicos_sem_t sem, unsigned long val)
{
    osal_semaphore_t sem_hdl = (osal_semaphore_t)sem;

    sem_hdl->count = val;

    return 0;
}

//--------------------------------------------------------------------+
// Mutex API (priority inheritance)
//--------------------------------------------------------------------+

static inline void aicos_mutex_init(aicos_mutex_t mutex)
{
    osal_semaphore_t m = (osal_semaphore_t)mutex;
    m->count = 1;
}

static inline aicos_mutex_t aicos_mutex_create(void)
{
    return (aicos_mutex_t)aicos_sem_create(1);
}

static inline void aicos_mutex_delete(aicos_mutex_t mutex)
{
    aicos_sem_delete(mutex);
}

static inline int aicos_mutex_take(aicos_mutex_t mutex, uint32_t msec)
{
    return aicos_sem_take(mutex, msec);
}

static inline int aicos_mutex_give(aicos_mutex_t mutex)
{
    return aicos_sem_give(mutex);
}

//--------------------------------------------------------------------+
// Event API
//--------------------------------------------------------------------+

typedef struct
{
  volatile unsigned int set;
}osal_event_def_t;

typedef osal_event_def_t* osal_event_t;

static inline aicos_event_t aicos_event_create(void)
{
    osal_event_t event;

    event = aicos_malloc(0, sizeof(osal_event_def_t));
    if (NULL == event)
        return NULL;

    event->set = 0;

    return (aicos_event_t)event;
}

static inline void aicos_event_delete(aicos_event_t event)
{
    aicos_free(0, event);
}

static inline int aicos_event_recv(aicos_event_t event, uint32_t set, uint32_t *recved, uint32_t msec)
{
    osal_event_t event_hdl = (osal_event_t)event;
    u64 s_us = 0;
    u64 e_us = 0;

    if (set == 0)
        return -EINVAL;

    if (event_hdl->set & set) {
        if (recved)
            *recved = (event_hdl->set & set);
        event_hdl->set &= ~set;
        return 0;
    }

    if (msec == 0) {
        return -EAGAIN;
    } else if (msec == AICOS_WAIT_FOREVER) {
        while (!(event_hdl->set & set)) { }
    } else {
        s_us = aic_get_time_us();
        while (!(event_hdl->set & set)) {
            e_us = aic_get_time_us();
            if (((e_us-s_us) / 1000) >= msec)
                return -ETIME;
        }
    }

    if (event_hdl->set & set) {
        if (recved)
            *recved = (event_hdl->set & set);
        event_hdl->set &= ~set;
        return 0;
    }

    return -EINVAL;
}

static inline int aicos_event_send(aicos_event_t event, uint32_t set)
{
    osal_event_t event_hdl = (osal_event_t)event;

    /* set event */
    event_hdl->set |= set;

    return 0;
}

//--------------------------------------------------------------------+
// Queue API
//--------------------------------------------------------------------+

typedef struct
{
    unsigned short depth;               /* max items */
    unsigned short item_size;           /* size of each item */
    volatile unsigned short wr_idx;     /* write pointer */
    volatile unsigned short rd_idx;     /* read pointer */
    unsigned char buffer[0];            /* data buffer */
}osal_queue_def_t;

typedef osal_queue_def_t* osal_queue_t;

/* ringbuffer */
#define RB_DATA_COUNT(w, r, max) (((w)>=(r)) ? ((w)-(r)) : ((max)-(r)+(w)))
#define RB_IS_FULL(w, r, max)    (RB_DATA_COUNT(w,r,max) >= ((max)-1))
#define RB_INC_POINTER(p, max)        p = ((p)+1)%(max)

static inline aicos_queue_t aicos_queue_create(uint32_t item_size, uint32_t queue_size)
{
    osal_queue_t q;

    q = aicos_malloc(0, sizeof(osal_queue_def_t) + item_size*queue_size);
    if (NULL == q)
        return NULL;

    q->item_size = item_size;
    q->depth = queue_size;
    q->rd_idx = 0;
    q->wr_idx = 0;

    return (aicos_queue_t)q;
}

static inline void aicos_queue_delete(aicos_queue_t queue)
{
    aicos_free(0, queue);
}

static inline int aicos_queue_send(aicos_queue_t queue, void const *buff)
{
    osal_queue_t q = (aicos_queue_t)queue;

    if (!buff)
        return -EINVAL;

    if (RB_IS_FULL(q->wr_idx, q->rd_idx, q->depth))
        return -EAGAIN;

    memcpy(q->buffer + q->wr_idx*q->item_size, buff, q->item_size);

    RB_INC_POINTER(q->wr_idx, q->depth);

    return 0;
}

static inline int aicos_queue_receive(aicos_queue_t queue, void *buff, uint32_t msec)
{
    osal_queue_t q = (aicos_queue_t)queue;
    u64 s_us = 0;
    u64 e_us = 0;

    if (!buff)
        return -EINVAL;

    if (RB_DATA_COUNT(q->wr_idx, q->rd_idx, q->depth)) {
        memcpy(buff, q->buffer + q->rd_idx*q->item_size, q->item_size);
        RB_INC_POINTER(q->rd_idx, q->depth);
        return 0;
    }

    if (msec == 0) {
        return -EAGAIN;
    } else if (msec == AICOS_WAIT_FOREVER) {
        while (!RB_DATA_COUNT(q->wr_idx, q->rd_idx, q->depth)) { }
    } else {
        s_us = aic_get_time_us();
        while (!RB_DATA_COUNT(q->wr_idx, q->rd_idx, q->depth)) {
            e_us = aic_get_time_us();
            if (((e_us-s_us) / 1000) >= msec)
                return -ETIME;
        }
    }

    if (RB_DATA_COUNT(q->wr_idx, q->rd_idx, q->depth)) {
        memcpy(buff, q->buffer + q->rd_idx*q->item_size, q->item_size);
        RB_INC_POINTER(q->rd_idx, q->depth);
        return 0;
    }

    return -EINVAL;
}

static inline int aicos_queue_empty(aicos_queue_t queue)
{
    osal_queue_t q = (aicos_queue_t)queue;

    return !RB_DATA_COUNT(q->wr_idx, q->rd_idx, q->depth);
}

//--------------------------------------------------------------------+
// Wait Queue API
//--------------------------------------------------------------------+

typedef struct
{
  unsigned int set;
  aicos_event_t event;
}osal_wqueue_def_t;

typedef osal_wqueue_def_t* osal_wqueue_t;

static inline aicos_wqueue_t aicos_wqueue_create(void)
{
    osal_wqueue_t wqueue;

    wqueue = aicos_malloc(0, sizeof(osal_wqueue_t));
    if (NULL == wqueue)
        return NULL;

    wqueue->event = aicos_event_create();
    wqueue->set = 0;

    return (aicos_wqueue_t)wqueue;
}

static inline void aicos_wqueue_delete(aicos_wqueue_t wqueue)
{
    osal_wqueue_t wq = (osal_wqueue_t)wqueue;
    aicos_event_t event = (aicos_event_t)wq->event;

    aicos_event_delete(event);
    aicos_free(0, wq);
}

static inline void aicos_wqueue_wakeup(aicos_wqueue_t wqueue)
{
    osal_wqueue_t wq = (osal_wqueue_t)wqueue;
    aicos_event_t event = (aicos_event_t)wq->event;

    if (wq->set != 1)
        return;

    wq->set = 0;
    aicos_event_send(event, 0x1);
}

static inline int aicos_wqueue_wait(aicos_wqueue_t wqueue, uint32_t msec)
{
    osal_wqueue_t wq = (osal_wqueue_t)wqueue;
    aicos_event_t event = (aicos_event_t)wq->event;
    uint32_t recved;

    wq->set = 1;
    return aicos_event_recv(event, 0x1, &recved, 100);
}

//--------------------------------------------------------------------+
// Critical API
//--------------------------------------------------------------------+
static inline size_t aicos_enter_critical_section(void)
{
    return (size_t)csi_irq_save();;
}
static inline void aicos_leave_critical_section(size_t flag)
{
    csi_irq_restore(flag);
}

//--------------------------------------------------------------------+
// Sleep API
//--------------------------------------------------------------------+
static inline void aicos_msleep(uint32_t delay)
{
    u64 s_us = 0;
    u64 e_us = 0;

    s_us = aic_get_time_us();
    while (1) {
        e_us = aic_get_time_us();
        if (((e_us-s_us) / 1000) >= delay)
            return;
    }
}

#ifdef __cplusplus
 }
#endif

#endif /* _ARTINCHIP_AIC_OSAL_BAREMETAL_H_ */
