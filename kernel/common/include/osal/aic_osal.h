/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_H_
#define _ARTINCHIP_AIC_OSAL_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <rtconfig.h>
#include <aic_common.h>

/* aicos_memcpy */
extern void *aicos_memcpy(void *, const void *, size_t);

/* Common malloc align */
typedef void *(*aicos_malloc1_t)(size_t);
typedef void (*aicos_free1_t)(void *);
typedef void *(*aicos_malloc2_t)(unsigned int, size_t);
typedef void (*aicos_free2_t)(unsigned int, void *);

void *_aicos_malloc_align_(size_t size, size_t align, unsigned int type, void *func);
void _aicos_free_align_(void *ptr, unsigned int type, void *func);

/* Irq */
void aicos_irq_enter(void);
void aicos_irq_exit(void);

/* New define */
typedef void *aicos_thread_t;
typedef void (*aic_thread_entry_t)(void *argument);
typedef void *aicos_sem_t;
typedef void *aicos_mutex_t;
typedef void *aicos_event_t;
typedef void *aicos_queue_t;
typedef void *aicos_wqueue_t;

#if defined(KERNEL_RTTHREAD)
#include "aic_osal_rtthread.h"
#elif defined(KERNEL_FREERTOS)
#include "aic_osal_freertos.h"
#elif defined(KERNEL_BAREMETAL)
#include "aic_osal_baremetal.h"
#elif defined(KERNEL_RHINO)
#include "aic_osal_rhino.h"
#elif defined(KERNEL_UCOS_II)
#include "aic_osal_ucos_ii.h"
#else
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
static inline aicos_sem_t aicos_sem_create(uint32_t initial_count) {return NULL;}
static inline void aicos_sem_delete(aicos_sem_t sem) {}
static inline int aicos_sem_take(aicos_sem_t sem, uint32_t msec) {return -1;}
static inline int aicos_sem_give(aicos_sem_t sem) {return -1;}

//--------------------------------------------------------------------+
// Mutex API (priority inheritance)
//--------------------------------------------------------------------+
static inline aicos_mutex_t aicos_mutex_create(void) {return NULL;}
static inline void aicos_mutex_delete(aicos_mutex_t mutex) {}
static inline int aicos_mutex_take(aicos_mutex_t mutex, uint32_t msec) {return -1;}
static inline int aicos_mutex_give(aicos_mutex_t mutex) {return -1;}

//--------------------------------------------------------------------+
// Event API
//--------------------------------------------------------------------+
static inline aicos_event_t aicos_event_create(void) {return NULL;}
static inline void aicos_event_delete(aicos_event_t event) {}
static inline int aicos_event_recv(aicos_event_t event, uint32_t set, uint32_t *recved, uint32_t msec) {return -1;}
static inline int aicos_event_send(aicos_event_t event, uint32_t set) {return -1;}

//--------------------------------------------------------------------+
// Queue API
//--------------------------------------------------------------------+
static inline aicos_queue_t aicos_queue_create(uint32_t item_size, uint32_t queue_size) {return NULL;}
static inline void aicos_queue_delete(aicos_queue_t queue) {}
static inline int aicos_queue_send(aicos_queue_t queue, void const *buff) {return -1;}
static inline int aicos_queue_receive(aicos_queue_t queue, void *buff, uint32_t msec) {return -1;}
static inline int aicos_queue_empty(aicos_queue_t queue) {return false;}

//--------------------------------------------------------------------+
// Wait Queue API
//--------------------------------------------------------------------+
static inline aicos_wqueue_t aicos_wqueue_create(aicos_wqueue_t queue) {return NULL;}
static inline void aicos_wqueue_delete(aicos_wqueue_t queue) {}
static inline int aicos_wqueue_wakeup(aicos_wqueue_t queue) {return -1;}
static inline int aicos_wqueue_wait(aicos_wqueue_t queue, uint32_t msec) {return -1;}

//--------------------------------------------------------------------+
// Critical API
//--------------------------------------------------------------------+
static inline size_t aicos_enter_critical_section(void) {return -1;}
static inline void aicos_leave_critical_section(size_t flag) {}

//--------------------------------------------------------------------+
// Sleep API
//--------------------------------------------------------------------+
static inline void aicos_msleep(uint32_t delay) {}
#endif


#if defined(PLATFORM_LUBANLITE)
#include "aic_osal_platform_lbl.h"
#elif defined(PLATFORM_YOC)
#include "aic_osal_platform_yoc.h"
#else
 //--------------------------------------------------------------------+
 // Irq API
 //--------------------------------------------------------------------+
 static inline void aicos_local_irq_save(unsigned long *state) {}
 static inline void aicos_local_irq_restore(unsigned long state) {}
 static inline void aicos_local_irq_disable(void) {}
 static inline void aicos_local_irq_enable(void) {}
 static inline int aicos_request_irq(unsigned int irq, irq_handler_t handler, unsigned int flags,
				  const char *name, void *dev) {return -1;}
 static inline void aicos_irq_enable(unsigned int irq) {}
 static inline void aicos_irq_disable(unsigned int irq) {}
 static inline void aicos_irq_enter(void) {}
 static inline void aicos_irq_exit(void) {}
 static inline bool aicos_in_irq(void) {return false;}

 //--------------------------------------------------------------------+
 // Cache API
 //--------------------------------------------------------------------+
 static inline void aicos_icache_enable(void) {}
 static inline void aicos_icache_disable(void) {}
 static inline void aicos_icache_invalid(void) {}
 static inline void aicos_dcache_enable(void) {}
 static inline void aicos_dcache_disable(void) {}
 static inline void aicos_dcache_invalid(void) {}
 static inline void aicos_dcache_clean(void) {}
 static inline void aicos_dcache_clean_invalid(void) {}
 static inline void aicos_dcache_invalid_range(unsigned long *addr, unsigned long size) {}
 static inline void aicos_dcache_clean_range(unsigned long *addr, unsigned long size) {}
 static inline void aicos_dcache_clean_invalid_range(unsigned long *addr, unsigned long size) {}
 //--------------------------------------------------------------------+
 // Delay API
 //--------------------------------------------------------------------+
 static inline void aicos_mdelay(unsigned long msecs) {}
 static inline void aicos_udelay(unsigned long usecs) {}
#endif // PLATFORM_LUBANLITE

// memory
void *aicos_malloc_try_cma(size_t size);
void *aicos_malloc_align_try_cma(size_t size, size_t align);

#ifdef __cplusplus
 }
#endif
#endif

