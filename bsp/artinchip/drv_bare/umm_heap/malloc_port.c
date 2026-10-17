/*
 * Copyright (C) 2023-2026 ArtInChip Technology Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Dehuang Wu <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <heap.h>
#include <aic_core.h>
#include "umm_malloc.h"

/* mem check */

#if defined(AIC_BOOTLOADER) && defined(AIC_BOOTLOADER_MEM_AUTO)
#if AIC_PSRAM_SIZE
    #if (AIC_BOOTLOADER_TEXT_BASE < CPU_PSRAM_BASE)
        #error AIC_BOOTLOADER_TEXT_BASE less than CPU_PSRAM_BASE
    #endif
    #if (AIC_BOOTLOADER_TEXT_BASE > (CPU_PSRAM_BASE + AIC_PSRAM_SIZE))
        #error AIC_BOOTLOADER_TEXT_BASE more than (CPU_PSRAM_BASE + AIC_PSRAM_SIZE)
    #endif
#elif AIC_DRAM_TOTAL_SIZE
    #if (AIC_BOOTLOADER_TEXT_BASE < CPU_DRAM_BASE)
        #error AIC_BOOTLOADER_TEXT_BASE less than CPU_DRAM_BASE
    #endif
    #if (AIC_BOOTLOADER_TEXT_BASE > (CPU_DRAM_BASE + AIC_DRAM_TOTAL_SIZE))
        #error AIC_BOOTLOADER_TEXT_BASE more than (CPU_DRAM_BASE + AIC_DRAM_TOTAL_SIZE)
    #endif
#elif AIC_SRAM_SIZE
    #if (AIC_BOOTLOADER_TEXT_BASE < CPU_SRAM_BASE)
        #error AIC_BOOTLOADER_TEXT_BASE less than CPU_SRAM_BASE
    #endif
    #if (AIC_BOOTLOADER_TEXT_BASE > (CPU_SRAM_BASE + AIC_SRAM_SIZE))
        #error AIC_BOOTLOADER_TEXT_BASE more than (CPU_SRAM_BASE + AIC_SRAM_SIZE)
    #endif
#elif AIC_SRAM_TOTAL_SIZE
    #if (AIC_BOOTLOADER_TEXT_BASE < CPU_SRAM_BASE)
        #error AIC_BOOTLOADER_TEXT_BASE less than CPU_SRAM_BASE
    #endif
    #if (AIC_BOOTLOADER_TEXT_BASE > (CPU_SRAM_BASE + AIC_SRAM_TOTAL_SIZE))
        #error AIC_BOOTLOADER_TEXT_BASE more than (CPU_SRAM_BASE + AIC_SRAM_TOTAL_SIZE)
    #endif
#endif
#endif

static struct umm_heap_config heap[MAX_MEM_REGION];

heap_def_t heap_def[MAX_MEM_REGION] = {
    {
        .name = "sys",
        .type = MEM_DEFAULT,
        .start = (size_t)(&__heap_start),
        .end = (size_t)(&__heap_end),
    },

#if defined(AIC_BOOTLOADER) && defined(AIC_BOOTLOADER_MEM_AUTO)
    {
        .name = "reserved",
        .type = MEM_RESERVED,
#if AIC_PSRAM_SIZE
        .start = (size_t)(CPU_PSRAM_BASE),
        .end = (size_t)(AIC_BOOTLOADER_TEXT_BASE - 0x100),
#elif AIC_DRAM_TOTAL_SIZE
        .start = (size_t)(CPU_DRAM_BASE),
        .end = (size_t)(AIC_BOOTLOADER_TEXT_BASE - 0x100),

#elif AIC_SRAM_SIZE || AIC_SRAM_TOTAL_SIZE
        .start = (size_t)(CPU_SRAM_BASE),
        .end = (size_t)(AIC_BOOTLOADER_TEXT_BASE - 0x100),
#endif
    },
#endif
};

int heap_init(void)
{
    int i = 0;
    size_t start, end;

    for (i = 0; i < MAX_MEM_REGION; i++) {
        start = heap_def[i].start;
        end = heap_def[i].end;
        if (start >= end) {
            pr_warn("%s: region %d addr err. start = 0x%x, end = 0x%x\n", __func__, i, (u32)start, (u32)end);
            return -1;
        }

        umm_init_heap(&heap[i], (void *)start, (end - start));
    }

    return 0;
}

#ifndef TLSF_MEM_HEAP
void *aic_tlsf_malloc(u32 mem_type, u32 nbytes)
{
    void *ptr;
    int i = 0;

    for (i = 0; i < sizeof(heap_def) / sizeof(heap_def_t); i++) {
        if (heap_def[i].type == mem_type)
            break;
    }

    if (i >= MAX_MEM_REGION) {
        pr_debug("%s: not found mem type %d, use mem type 0.\n", __func__, mem_type);
        ptr = umm_malloc(&heap[0], nbytes);
        return ptr;
    }

    ptr = umm_malloc(&heap[i], nbytes);

    pr_debug("%s: ptr = 0x%x, nbytes = 0x%x.\n", __func__, (u32)(uintptr_t)ptr, nbytes);

    return ptr;
}

void aic_tlsf_free(u32 mem_type, void *ptr)
{
    int i = 0;

    for (i = 0; i < sizeof(heap_def) / sizeof(heap_def_t); i++) {
        if (heap_def[i].type == mem_type)
            break;
    }

    pr_debug("%s: ptr = 0x%x.\n", __func__, (u32)(uintptr_t)ptr);

    if (i >= MAX_MEM_REGION) {
        pr_debug("%s: not found mem type %d, use mem type 0.\n", __func__, mem_type);
        umm_free(&heap[0], ptr);
        return;
    }

    umm_free(&heap[i], ptr);
}

void *aic_tlsf_malloc_align(u32 mem_type, u32 nbytes, u32 align)
{
    void *ptr;
    int i = 0;

    for (i = 0; i < sizeof(heap_def) / sizeof(heap_def_t); i++) {
        if (heap_def[i].type == mem_type)
            break;
    }

    if (i >= MAX_MEM_REGION) {
        pr_debug("%s: not found mem type %d, use mem type 0.\n", __func__, mem_type);
        ptr = umm_malloc_align(&heap[0], nbytes, align);
        return ptr;
    }

    ptr = umm_malloc_align(&heap[i], nbytes, align);

    pr_debug("%s: ptr = 0x%x, nbytes = 0x%x.\n", __func__, (u32)(uintptr_t)ptr, nbytes);

    return ptr;
}

void aic_tlsf_free_align(u32 mem_type, void *ptr)
{
    int i = 0;

    for (i = 0; i < sizeof(heap_def) / sizeof(heap_def_t); i++) {
        if (heap_def[i].type == mem_type)
            break;
    }

    pr_debug("%s: ptr = 0x%x.\n", __func__, (u32)(uintptr_t)ptr);

    if (i >= MAX_MEM_REGION) {
        pr_debug("%s: not found mem type %d, use mem type 0.\n", __func__, mem_type);
        umm_free_align(&heap[0], ptr);
        return;
    }

    umm_free_align(&heap[i], ptr);
}

void *aic_tlsf_realloc(u32 mem_type, void *ptr, u32 nbytes)
{
    int i = 0;

    for (i = 0; i < sizeof(heap_def) / sizeof(heap_def_t); i++) {
        if (heap_def[i].type == mem_type)
            break;
    }

    if (i >= MAX_MEM_REGION) {
        pr_debug("%s: not found mem type %d, use mem type 0.\n", __func__, mem_type);
        ptr = umm_realloc(&heap[0], ptr, nbytes);
        return ptr;
    }

    ptr = umm_realloc(&heap[i], ptr, nbytes);

    pr_debug("%s: ptr = 0x%x, nbytes = 0x%x.\n", __func__, (u32)(uintptr_t)ptr, nbytes);

    return ptr;
}

void *aic_tlsf_calloc(u32 mem_type, u32 count, u32 nbytes)
{
    void *ptr;
    int i = 0;

    for (i = 0; i < sizeof(heap_def) / sizeof(heap_def_t); i++) {
        if (heap_def[i].type == mem_type)
            break;
    }

    if (i >= MAX_MEM_REGION) {
        pr_debug("%s: not found mem type %d, use mem type 0.\n", __func__, mem_type);
        ptr = umm_calloc(&heap[0], count, nbytes);
        return ptr;
    }

    ptr = umm_calloc(&heap[i], count, nbytes);

    pr_debug("%s: ptr = 0x%x, nbytes = 0x%x.\n", __func__, (u32)(uintptr_t)ptr, nbytes);

    return ptr;
}

#endif
