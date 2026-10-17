/*
 * Copyright (C) 2020-2026 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: mpp thread interface
 */

#include "mpp_thread.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include "rtconfig.h"

#if defined(KERNEL_RTTHREAD) || \
    defined(KERNEL_FREERTOS) || \
    defined(KERNEL_BAREMETAL) || \
    defined(KERNEL_RHINO) || \
    defined(KERNEL_UCOS_II)
#define MPP_USING_AIC_OSAL
#endif


#if defined(MPP_USING_AIC_OSAL)
#include "aic_osal.h"
struct mpp_sem_handle {
    aicos_sem_t sem;
};

struct mpp_thread_handle {
    aicos_thread_t thread;
};
#else
#include <pthread.h>
#include <sys/prctl.h>
#include <errno.h>
#include <time.h>
struct mpp_sem_handle {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_condattr_t condattr;
    int need_signal;
};
struct mpp_thread_handle {
    pthread_t tid;
    char name[16];
    mpp_thread_entry_t entry;
    void *args;

    /* suspend / resume */
    pthread_mutex_t lock;
    pthread_cond_t resume_cond;
    int suspended;
    int should_stop;
};

static void *mpp_thread_start(void *arg)
{
    struct mpp_thread_handle *h_thread = (struct mpp_thread_handle *)arg;

    prctl(PR_SET_NAME, h_thread->name, 0, 0, 0);
    h_thread->entry(h_thread->args);
    return NULL;
}


#endif /* MPP_USING_AIC_OSAL */


/* -------------------------------------------------------------------------- */
/*  Thread API                                                                */
/* -------------------------------------------------------------------------- */

mpp_thread_t mpp_thread_create(const char *name, uint32_t stack_size, uint32_t prio,
                   mpp_thread_entry_t entry, void *args)
{
    struct mpp_thread_handle *h_thread =
        (struct mpp_thread_handle *)mpp_alloc(sizeof(struct mpp_thread_handle));

    if (!h_thread)
        return NULL;
    memset(h_thread, 0, sizeof(struct mpp_thread_handle));

#if defined(MPP_USING_AIC_OSAL)
    h_thread->thread = aicos_thread_create(name, stack_size, prio,
                           (aic_thread_entry_t)entry, args);
    if (!h_thread->thread) {
        loge("aicos_thread_create failed");
        mpp_free(h_thread);
        return NULL;
    }
#else
    pthread_attr_t attr;
    int ret;

    if (name)
        strncpy(h_thread->name, name, sizeof(h_thread->name) - 1);

    h_thread->entry = entry;
    h_thread->args = args;

    pthread_mutex_init(&h_thread->lock, NULL);
    pthread_cond_init(&h_thread->resume_cond, NULL);

    pthread_attr_init(&attr);
    if (stack_size > 0)
        pthread_attr_setstacksize(&attr, stack_size);
    (void)prio;

    ret = pthread_create(&h_thread->tid, &attr, mpp_thread_start, h_thread);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        loge("pthread_create failed: %d", ret);
        pthread_mutex_destroy(&h_thread->lock);
        pthread_cond_destroy(&h_thread->resume_cond);
        mpp_free(h_thread);
        return NULL;
    }
#endif
    return (mpp_thread_t)h_thread;
}

void mpp_thread_delete(mpp_thread_t thread)
{
    struct mpp_thread_handle *h_thread = (struct mpp_thread_handle *)thread;
    if (!thread)
        return;

#if defined(MPP_USING_AIC_OSAL)
    aicos_thread_delete(h_thread->thread);
#else
    pthread_join(h_thread->tid, NULL);
    pthread_mutex_destroy(&h_thread->lock);
    pthread_cond_destroy(&h_thread->resume_cond);
#endif
    mpp_free(h_thread);
}

int mpp_thread_suspend(mpp_thread_t thread)
{
    struct mpp_thread_handle *h_thread = (struct mpp_thread_handle *)thread;
    if (!thread)
        return -1;

#if defined(MPP_USING_AIC_OSAL)
    aicos_thread_suspend(h_thread->thread);
    return 0;
#else
    return -1;
#endif
}

int mpp_thread_resume(mpp_thread_t thread)
{
    struct mpp_thread_handle *h_thread = (struct mpp_thread_handle *)thread;
    if (!thread)
        return -1;

#if defined(MPP_USING_AIC_OSAL)
    aicos_thread_resume(h_thread->thread);
#else
    pthread_mutex_lock(&h_thread->lock);
    if (h_thread->suspended) {
        h_thread->suspended = 0;
        pthread_cond_signal(&h_thread->resume_cond);
    }
    pthread_mutex_unlock(&h_thread->lock);
#endif
    return 0;
}


/* -------------------------------------------------------------------------- */
/*  Semaphore API                                                             */
/* -------------------------------------------------------------------------- */
mpp_sem_t mpp_sem_create()
{
    struct mpp_sem_handle *h_sem =
        (struct mpp_sem_handle *)mpp_alloc(sizeof(struct mpp_sem_handle));
    if (h_sem == NULL)
        return NULL;
    memset(h_sem, 0, sizeof(struct mpp_sem_handle));

#if defined(MPP_USING_AIC_OSAL)
    h_sem->sem = aicos_sem_create(0);
    if (!h_sem->sem) {
        loge("aicos_sem_create err!!!\n");
        mpp_free(h_sem);
        return NULL;
    }
#else
    h_sem->need_signal = 0;
    if (pthread_mutex_init(&h_sem->mutex, NULL) != 0) {
        loge("pthread_mutex_init err!!!\n");
        mpp_free(h_sem);
        return NULL;
    }

    pthread_condattr_init(&h_sem->condattr);
    pthread_condattr_setclock(&h_sem->condattr, CLOCK_REALTIME);
    pthread_cond_init(&h_sem->cond, &h_sem->condattr);
#endif
    return (mpp_sem_t)h_sem;
}

void mpp_sem_delete(mpp_sem_t sem)
{
    if (sem) {
        struct mpp_sem_handle *h_sem = (struct mpp_sem_handle *)sem;
#if defined(MPP_USING_AIC_OSAL)
        aicos_sem_delete(h_sem->sem);
        h_sem->sem = NULL;
#else
        pthread_condattr_destroy(&h_sem->condattr);
        pthread_cond_destroy(&h_sem->cond);
        pthread_mutex_destroy(&h_sem->mutex);
#endif
        mpp_free(sem);
    }
}

int mpp_sem_signal(mpp_sem_t sem)
{
    if (!sem)
        return -1;
    struct mpp_sem_handle *h_sem = (struct mpp_sem_handle *)sem;
#if defined(MPP_USING_AIC_OSAL)
    aicos_sem_give(h_sem->sem);
#else
    pthread_mutex_lock(&h_sem->mutex);
    if (h_sem->need_signal) {
        pthread_cond_signal(&h_sem->cond);
    }
    pthread_mutex_unlock(&h_sem->mutex);
#endif
    return 0;
}

int mpp_sem_wait(mpp_sem_t sem, int64_t us)
{
    if (!sem)
        return -1;

    struct mpp_sem_handle *h_sem = (struct mpp_sem_handle *)sem;

#if defined(MPP_USING_AIC_OSAL)
    if (us == MPP_WAIT_FOREVER)
        aicos_sem_take(h_sem->sem, AICOS_WAIT_FOREVER);
    else
        aicos_sem_take(h_sem->sem, (unsigned int)(us * 1000));
    return 0;
#else
    int ret;
    pthread_mutex_lock(&h_sem->mutex);
    if (us == MPP_WAIT_FOREVER) {
        h_sem->need_signal = 1;
        ret = pthread_cond_wait(&h_sem->cond, &h_sem->mutex);
        h_sem->need_signal = 0;
    } else {
        struct timespec out_time;
        uint64_t tmp;
        ret = clock_gettime(CLOCK_REALTIME, &out_time);
        out_time.tv_sec += us / (1 * 1000 * 1000);
        tmp = out_time.tv_nsec / 1000 + us % (1 * 1000 * 1000);
        out_time.tv_sec += tmp / (1 * 1000 * 1000);
        tmp = tmp % (1 * 1000 * 1000);
        out_time.tv_nsec = tmp * 1000;
        h_sem->need_signal = 1;
        ret = pthread_cond_timedwait(&h_sem->cond, &h_sem->mutex, &out_time);
        h_sem->need_signal = 0;
    }

    pthread_mutex_unlock(&h_sem->mutex);
    return ret;
#endif
}
