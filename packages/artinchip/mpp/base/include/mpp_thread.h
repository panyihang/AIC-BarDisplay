/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: mpp thread interface
 */

#ifndef MPP_THREAD_H
#define MPP_THREAD_H

#include <stdint.h>
#include <inttypes.h>
#include "mpp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPP_WAIT_FOREVER  (-1)

typedef void *mpp_sem_t;
typedef void *mpp_thread_t;
typedef void (*mpp_thread_entry_t)(void *argument);

/* Semaphore */
mpp_sem_t mpp_sem_create(void);
void mpp_sem_delete(mpp_sem_t sem);
int mpp_sem_signal(mpp_sem_t sem);
int mpp_sem_wait(mpp_sem_t sem, int64_t us);

/* Thread */
mpp_thread_t mpp_thread_create(const char *name, uint32_t stack_size, uint32_t prio,
                               mpp_thread_entry_t entry, void *args);
void mpp_thread_delete(mpp_thread_t thread);
int mpp_thread_suspend(mpp_thread_t thread);
int mpp_thread_resume(mpp_thread_t thread);

#ifdef __cplusplus
}
#endif

#endif /* MPP_THREAD_H */
