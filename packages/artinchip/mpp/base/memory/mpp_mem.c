/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: qi.xu@artinchip.com
 * Desc: virtual memory allocator
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "aic_core.h"
#include "mpp_mem.h"
#include "mpp_log.h"

#define DEBUG_MEM  (0)

#if DEBUG_MEM
#include <pthread.h>
#define MPP_MEMINFO_MAX_NUM  1024
struct memory_node {
    void* addr;
    int size;
    int used;
    int line;
    const char *file_name;
};

static pthread_mutex_t  g_mem_mutex = PTHREAD_MUTEX_INITIALIZER;
int                     g_mem_count = 0;
int                     g_mem_total_size = 0;
struct memory_node	g_mem_info[MPP_MEMINFO_MAX_NUM] = {0};

#endif

void *_mpp_alloc_(size_t len,const char *file,int line)
{
    void *ptr = NULL;

    ptr = malloc(len);

#if DEBUG_MEM
    int i;
    pthread_mutex_lock(&g_mem_mutex);
    g_mem_count ++;
    g_mem_total_size += len;
    for(i=0; i< MPP_MEMINFO_MAX_NUM; i++) {
        if(g_mem_info[i].used == 0) {
            g_mem_info[i].addr = ptr;
            g_mem_info[i].size = len;
            g_mem_info[i].used = 1;
            g_mem_info[i].file_name= file;
            g_mem_info[i].line = line;

            printf("alloc[%s:%d]ptr:%p,len:%d\n"
                ,g_mem_info[i].file_name
                ,g_mem_info[i].line
                ,g_mem_info[i].addr
                ,g_mem_info[i].size);


            break;
        }
    }
    if(i == MPP_MEMINFO_MAX_NUM) {
        loge("exceed max number");
    }
    pthread_mutex_unlock(&g_mem_mutex);
#endif
    return ptr;
}

void mpp_free(void *ptr)
{
    if(ptr == NULL)
        return;

    free(ptr);

#if DEBUG_MEM
    int i;
    pthread_mutex_lock(&g_mem_mutex);
    for(i=0; i<MPP_MEMINFO_MAX_NUM; i++) {
        if(g_mem_info[i].used && g_mem_info[i].addr == ptr) {
            g_mem_count--;
            g_mem_total_size -= g_mem_info[i].size;
            g_mem_info[i].used = 0;
            printf("free[%s:%d]ptr:%p,len:%d\n"
            ,g_mem_info[i].file_name
            ,g_mem_info[i].line
            ,g_mem_info[i].addr
            ,g_mem_info[i].size);
            break;
        }
    }
    if(i == MPP_MEMINFO_MAX_NUM) {
        loge("cannot find this memory");
    }
    pthread_mutex_unlock(&g_mem_mutex);
#endif
}

void *mpp_realloc(void *ptr,size_t size)
{
	return realloc(ptr,size);
}

void show_mem_info_debug()
{
#if DEBUG_MEM
    int i;
    loge("total memory size: %d,g_mem_count:%d\n", g_mem_total_size,g_mem_count);
    for(i=0; i<MPP_MEMINFO_MAX_NUM; i++) {
        if(g_mem_info[i].used) {
            printf("memleak :file:%s,line:%d, addr: %p, %d\n"
            ,g_mem_info[i].file_name
            ,g_mem_info[i].line
            ,g_mem_info[i].addr
            ,g_mem_info[i].size);
        }
    }
#endif
}

/***************************** physic memory *****************************************/
#define DEBUG_PHY_MEM (0)

#if DEBUG_PHY_MEM
#include <pthread.h>
#define MEMORY_NUM 48
struct phy_mem_info {
    unsigned int addr;
    int size;
    int used;
};
static pthread_mutex_t  g_phy_mem_mutex = PTHREAD_MUTEX_INITIALIZER;
struct phy_mem_info info[MEMORY_NUM];
int total_cnt = 0;
#endif

unsigned int mpp_phy_alloc(size_t size)
{
    unsigned int addr = (unsigned long)aicos_malloc_align_try_cma(size, 1024);
    if (addr == 0) {
        loge("mpp_phy_alloc failed");
        return 0;
    }
    aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)addr), size);

#if DEBUG_PHY_MEM
    int i;
    pthread_mutex_lock(&g_phy_mem_mutex);
    for(i=0; i<MEMORY_NUM; i++) {
        if(info[i].used == 0)
            break;
    }

    if(i == MEMORY_NUM) {
        loge("memory count exceed max number");
        pthread_mutex_unlock(&g_phy_mem_mutex);
        return 0;
    }

    info[i].addr = addr;
    info[i].size = size;
    info[i].used = 1;
    total_cnt ++;
    pthread_mutex_unlock(&g_phy_mem_mutex);
#endif

    logw("mpp_phy_alloc success, addr: %08x, size: %d", addr, (int)size);
    return addr;
}

void mpp_phy_free(unsigned int addr)
{
#if DEBUG_PHY_MEM
    int i;
    pthread_mutex_lock(&g_phy_mem_mutex);
    for(i=0; i<MEMORY_NUM; i++) {
        if(info[i].addr == addr) {
            if(info[i].used == 0) {
                pthread_mutex_unlock(&g_phy_mem_mutex);
                loge("the buffer not alloc, addr: %08x, size: %d", addr, info[i].size);
                return;
            }
            break;
        }
    }

    if(i == MEMORY_NUM) {
        pthread_mutex_unlock(&g_phy_mem_mutex);
        loge("not found the addr: %08x, maybe error!", addr);
        return;
    }

    info[i].used = 0;
    total_cnt --;
    pthread_mutex_unlock(&g_phy_mem_mutex);
#endif

    if (addr == 0) {
        loge("mpp_phy_free addr is 0, maybe error");
        return;
    }
    aicos_free_align(MEM_CMA, (void*)(unsigned long)addr);

    logw("phy_free success, addr: %08x", addr);
}
